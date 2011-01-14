/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include "peermanager.h"

#include <QtAlgorithms>
#include <QFile>
#include <QList>
#include <QSet>
#include <QTextStream>
#include <QDateTime>
#include <k3resolver.h>
#include <klocale.h>

#include <util/ptrmap.h>
#include <peer/peer.h>
#include <peer/peerid.h>
#include <util/bitset.h>
#include <util/log.h>
#include <util/file.h>
#include <util/error.h>
#include <net/address.h>
#include <torrent/torrent.h>
#include <util/functions.h>
#include <mse/streamsocket.h> 
#include <mse/encryptedauthenticate.h>
#include <peer/accessmanager.h>
#include <torrent/globals.h>
#include <torrent/server.h>
#include <dht/dhtbase.h>
#include "packetwriter.h"
#include "chunkcounter.h"
#include "authenticationmonitor.h"
#include "peer.h"
#include "authenticate.h"
#include "peerconnector.h"

using namespace KNetwork;

namespace bt
{
	Uint32 PeerManager::max_connections = 0;
	Uint32 PeerManager::max_total_connections = 0;
	Uint32 PeerManager::total_connections = 0;
	
	typedef std::multimap<QString,PotentialPeer>::iterator PPItr;
	
	class PeerManager::Private
	{
	public:
		Private(PeerManager* p, Torrent & tor);
		~Private();
		
		void updateAvailableChunks();
		bool killBadPeer();
		void createPeer(mse::StreamSocket::Ptr sock,const PeerID & peer_id,Uint32 support,bool local);
		bool connectedTo(const QString & ip,Uint16 port) const;
		void update();
		void have(Peer* peer,Uint32 index);
		void connectToPeers();
		
	public:
		PeerManager* p;
		PtrMap<Uint32,Peer> peer_map;
		QList<Peer*> peer_list;
		QList<Peer*> killed;
		Torrent & tor;
		bool started;
		BitSet available_chunks, wanted_chunks;
		ChunkCounter* cnt;
		bool pex_on;
		bool wanted_changed;
		PieceHandler* piece_handler;
		bool paused;
		QSet<PeerConnector::Ptr> connectors;
		SuperSeeder* superseeder;
		std::multimap<QString,PotentialPeer> potential_peers;
	};

	PeerManager::PeerManager(Torrent & tor)
		: d(new Private(this,tor))
	{
	}


	PeerManager::~PeerManager()
	{
		delete d;
	}

	void PeerManager::pause()
	{
		if (d->paused)
			return;
		
		foreach (Peer* p,d->peer_list)
		{
			p->pause();
		}
		d->paused = true;
	}

	void PeerManager::unpause()
	{
		if (!d->paused)
			return;
		
		foreach (Peer* p,d->peer_list)
		{
			p->unpause();
			if (p->hasWantedChunks(d->wanted_chunks)) // send interested when it has wanted chunks
				p->getPacketWriter().sendInterested();
		}
		d->paused = false;
	}

	void PeerManager::update()
	{
		d->update();
	}
	
	void PeerManager::setMaxConnections(Uint32 max)
	{
		max_connections = max;
	}
	
	void PeerManager::setMaxTotalConnections(Uint32 max)
	{
#ifndef Q_WS_WIN
		Uint32 sys_max = bt::MaxOpenFiles() - 50; // leave about 50 free for regular files
#else
		Uint32 sys_max = 9999; // there isn't a real limit on windows
#endif
		max_total_connections = max;
		if (max == 0 || max_total_connections > sys_max)
			max_total_connections = sys_max;
	}
	
	void PeerManager::setWantedChunks(const BitSet & bs)
	{
		d->wanted_chunks = bs;
		d->wanted_changed = true;
	}
	
	void PeerManager::addPotentialPeer(const PotentialPeer & pp)
	{
		if (d->potential_peers.size() > 500)
			return;
		
		KIpAddress addr;
		if (addr.setAddress(pp.ip))
		{
			// avoid duplicates in the potential_peers map
			std::pair<PPItr,PPItr> r = d->potential_peers.equal_range(pp.ip);
			for (PPItr i = r.first;i != r.second;i++)
			{
				if (i->second.port == pp.port) // port and IP are the same so return
					return;
			}
		
			d->potential_peers.insert(std::make_pair(pp.ip,pp));
		}
		else
		{
			// must be a hostname, so resolve it
			KResolver::resolveAsync(this,SLOT(onResolverResults(KNetwork::KResolverResults )),
					pp.ip,QString::number(pp.port));
		}
	}
	
	void PeerManager::onResolverResults(KResolverResults res)
	{
		if (res.count() == 0)
			return;
		
		net::Address addr = res.front().address().asInet();
		
		PotentialPeer pp;
		pp.ip = addr.ipAddress().toString();
		pp.port = addr.port();
		pp.local = false;
		
		// avoid duplicates in the potential_peers map
		std::pair<PPItr,PPItr> r = d->potential_peers.equal_range(pp.ip);
		for (PPItr i = r.first;i != r.second;i++)
		{
			if (i->second.port == pp.port) // port and IP are the same so return
				return;
		}
		
		d->potential_peers.insert(std::make_pair(pp.ip,pp));
	}

	void PeerManager::killSeeders()
	{
		QList<Peer*>::iterator i = d->peer_list.begin();
		while (i != d->peer_list.end())
		{
			Peer* p = *i;
 			if ( p->isSeeder() )
 				p->kill();
			i++;
		}
	}
	
	void PeerManager::killUninterested()
	{
		QList<Peer*>::iterator i = d->peer_list.begin();
		while (i != d->peer_list.end())
		{
			Peer* p = *i;
			if ( !p->isInterested() && (p->getConnectTime().secsTo(QTime::currentTime()) > 30) )
				p->kill();
			i++;
		}
	}

	void PeerManager::have(Peer* p,Uint32 index)
	{
		d->have(p,index);
	}

	void PeerManager::bitSetReceived(Peer* p, const BitSet & bs)
	{
		bool interested = false;
		for (Uint32 i = 0;i < bs.getNumBits();i++)
		{
			if (bs.get(i))
			{
				if (d->wanted_chunks.get(i))
					interested = true;
				d->available_chunks.set(i,true);
				d->cnt->inc(i);
			}
		}
		
		if (interested && !d->paused)
			p->getPacketWriter().sendInterested();
		
		if (d->superseeder)
			d->superseeder->bitset(p,bs);
	}
	

	void PeerManager::newConnection(mse::StreamSocket::Ptr sock,const PeerID & peer_id,Uint32 support)
	{
		if (!d->started)
			return;
		
		Uint32 total = d->peer_list.count() + d->connectors.size();
		bool local_not_ok = (max_connections > 0 && total >= max_connections);
		bool global_not_ok = (max_total_connections > 0 && total_connections >= max_total_connections);
		
		if (local_not_ok || global_not_ok)
		{
			// get rid of bad peer and replace it by another one
			if (!d->killBadPeer())
			{
				// we failed to find a bad peer, so just delete this one
				return;
			}
		}

		d->createPeer(sock,peer_id,support,false);
	}
	
	void PeerManager::peerAuthenticated(Authenticate* auth,PeerConnector::WPtr pcon,bool ok)
	{
		if (d->started)
		{
			if (total_connections > 0)
				total_connections--;
			
			if (ok && !connectedTo(auth->getPeerID()))
				d->createPeer(auth->getSocket(),auth->getPeerID(),auth->supportedExtensions(),auth->isLocal());
		}
		
		PeerConnector::Ptr ptr = pcon.toStrongRef();
		d->connectors.remove(ptr);
	}
	

		
	bool PeerManager::connectedTo(const PeerID & peer_id)
	{
		if (!d->started)
			return false;
		
		foreach (Peer* p,d->peer_list)
		{
			if (p->getPeerID() == peer_id)
			{
				return true;
			}
		}
		return false;
	}
	
	void PeerManager::connectToPeers()
	{
		d->connectToPeers();
	}
	
	Uint32 PeerManager::clearDeadPeers()
	{
		Uint32 num = d->killed.count();
		qDeleteAll(d->killed);
		d->killed.clear();
		return num;
	}
	
	void PeerManager::closeAllConnections()
	{
		qDeleteAll(d->killed);
		d->killed.clear();
	
		if ((Uint32)d->peer_list.count() <= total_connections)
			total_connections -= d->peer_list.count();
		else
			total_connections = 0;

		d->peer_map.clear();
		qDeleteAll(d->peer_list);
		d->peer_list.clear();
	}
	
	
	void PeerManager::savePeerList(const QString & file)
	{
		// Lets save the entries line based
		QFile fptr(file);
		if (!fptr.open(QIODevice::WriteOnly))
			return;
		
		
		try
		{
			Out(SYS_GEN|LOG_DEBUG) << "Saving list of peers to " << file << endl;
			
			QTextStream out(&fptr);
			// first the active peers
			foreach(Peer* p,d->peer_list)
			{
				const net::Address & addr = p->getAddress();
				out << addr.ipAddress().toString() << " " << (unsigned short)addr.port() << ::endl;
			}
			
			// now the potential_peers
			PPItr i = d->potential_peers.begin();
			while (i != d->potential_peers.end())
			{
				out << i->first << " " <<  i->second.port << ::endl;
				i++;
			}
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Error happened during saving of peer list : " << err.toString() << endl;
		}
	}
	
	void PeerManager::loadPeerList(const QString & file)
	{
		QFile fptr(file);
		if (!fptr.open(QIODevice::ReadOnly))
			return;
		
		try
		{
			
			Out(SYS_GEN|LOG_DEBUG) << "Loading list of peers from " << file  << endl;
			
			while (!fptr.atEnd())
			{
				QStringList sl = QString(fptr.readLine()).split(" ");
				if (sl.count() != 2)
					continue;
				
				bool ok = false;
				PotentialPeer pp;
				pp.ip = sl[0];
				pp.port = sl[1].toInt(&ok);
				if (ok)
					addPotentialPeer(pp);
			}
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Error happened during saving of peer list : " << err.toString() << endl;
		}
	}
	
	void PeerManager::start(bool superseed)
	{
		d->started = true;
		if (superseed && !d->superseeder)
			d->superseeder = new SuperSeeder(d->cnt->getNumChunks(),this);
		
		unpause();
		ServerInterface::addPeerManager(this);
	}
		
	
	void PeerManager::stop()
	{
		d->cnt->reset();
		d->available_chunks.clear();
		d->started = false;
		ServerInterface::removePeerManager(this);
		d->connectors.clear();
		
		if (d->superseeder)
		{
			delete d->superseeder;
			d->superseeder = 0;
		}
		
		closeAllConnections();
	}

	Peer* PeerManager::findPeer(Uint32 peer_id)
	{
		return d->peer_map.find(peer_id);
	}
	
	Peer* PeerManager::findPeer(PieceDownloader* pd)
	{
		foreach (Peer* p,d->peer_list)
		{
			if ((PieceDownloader*)p->getPeerDownloader() == pd)
				return p;
		}
		return 0;
	}
	
	void PeerManager::rerunChoker()
	{
		// append a 0 ptr to killed
		// so that the next update in TorrentControl
		// will be forced to do the choking
		d->killed.append(0);
	}
	
	
	
	void PeerManager::peerSourceReady(PeerSource* ps)
	{
		PotentialPeer pp;
		while (ps->takePotentialPeer(pp))
			addPotentialPeer(pp);
	}
	
	
	
	void PeerManager::pex(const QByteArray & arr)
	{
		if (!d->pex_on)
			return;
		
		Out(SYS_CON|LOG_NOTICE) << "PEX: found " << (arr.size() / 6) << " peers"  << endl;
		for (int i = 0;i+6 <= arr.size();i+=6)
		{
			Uint8 tmp[6];
			memcpy(tmp,arr.data() + i,6);
			PotentialPeer pp;
			pp.port = ReadUint16(tmp,4);
			Uint32 ip = ReadUint32(tmp,0);
			pp.ip = QString("%1.%2.%3.%4")
						.arg((ip & 0xFF000000) >> 24)
						.arg((ip & 0x00FF0000) >> 16)
						.arg((ip & 0x0000FF00) >> 8)
						.arg( ip & 0x000000FF);
			pp.local = false;
			
			addPotentialPeer(pp);
		}
	}
	
	
	void PeerManager::setPexEnabled(bool on)
	{
		if (on && d->tor.isPrivate())
			return;
		
		if (d->pex_on == on)
			return;
		
		foreach (Peer* p,d->peer_list)
		{
			if (!p->isKilled())
			{
				p->setPexEnabled(on);
				bt::Uint16 port = ServerInterface::getPort();
				p->sendExtProtHandshake(port,d->tor.getMetaData().size());
			}
		}
		d->pex_on = on;
	}
	
	void PeerManager::setGroupIDs(Uint32 up,Uint32 down)
	{
		for (PtrMap<Uint32,Peer>::iterator i = d->peer_map.begin();i != d->peer_map.end();i++)
		{
			Peer* p = i->second;
			p->setGroupIDs(up,down);
		}
	}
		
	void PeerManager::portPacketReceived(const QString& ip, Uint16 port)
	{
		if (Globals::instance().getDHT().isRunning() && !d->tor.isPrivate())
			Globals::instance().getDHT().portReceived(ip,port);
	}
	
	void PeerManager::pieceReceived(const Piece & p)
	{
		if (d->piece_handler)
			d->piece_handler->pieceReceived(p);
	}
		
	void PeerManager::setPieceHandler(PieceHandler* ph)
	{
		d->piece_handler = ph;
	}
	
	void PeerManager::killStalePeers()
	{
		foreach (Peer* p,d->peer_list)
		{
			if (p->getDownloadRate() == 0 && p->getUploadRate() == 0)
				p->kill();
		}
	}
	
	void PeerManager::setSuperSeeding(bool on,const BitSet & chunks)
	{
		Q_UNUSED(chunks);
		if ((d->superseeder && on) || (!d->superseeder && !on))
			return;
		
		if (on)
		{
			d->superseeder = new SuperSeeder(d->cnt->getNumChunks(),this);
		}
		else
		{
			delete d->superseeder;
			d->superseeder = 0;
		}
		
		// When entering or exiting superseeding mode kill all peers 
		// but first add the current list to the potential_peers list, so we can reconnect later.
		foreach(Peer* p,d->peer_list)
		{
			const net::Address & addr = p->getAddress();
			PotentialPeer pp;
			pp.ip = addr.ipAddress().toString();
			pp.port = addr.port();
			pp.local = false;
			d->potential_peers.insert(std::make_pair(pp.ip,pp));
			p->kill();
		}
	}
	
	void PeerManager::allowChunk(PeerInterface* peer, Uint32 chunk)
	{
		Out(SYS_CON|LOG_DEBUG) << "Peer " << peer->getPeerID().toString() << " is allowed to download " << chunk << endl;
		Peer* p = dynamic_cast<Peer*>(peer);
		if (p)
			p->getPacketWriter().sendHave(chunk);
	}
	
	void PeerManager::sendHave(Uint32 index)
	{
		if (d->superseeder)
			return;
		
		foreach (Peer* peer, d->peer_list)
		{
			peer->getPacketWriter().sendHave(index);
		}
	}
	
	Uint32 PeerManager::getNumConnectedPeers() const 
	{
		return d->peer_list.count();
	}
	
	Uint32 PeerManager::getNumPending() const 
	{
		return d->connectors.size();
	}
	
	const bt::BitSet& PeerManager::getAvailableChunksBitSet() const
	{
		return d->available_chunks;
	}
	
	ChunkCounter& PeerManager::getChunkCounter()
	{
		return *d->cnt;
	}

	bool PeerManager::isPexEnabled() const
	{
		return d->pex_on;
	}

	const bt::Torrent& PeerManager::getTorrent() const
	{
		return d->tor;
	}
	
	bool PeerManager::isStarted() const
	{
		return d->started;
	}

	void PeerManager::visit(PeerManager::PeerVisitor& visitor)
	{
		foreach (const Peer* p,d->peer_list)
			visitor.visit(p);
	}
	
	Uint32 PeerManager::uploadRate() const
	{
		Uint32 rate = 0;
		foreach (const Peer* p,d->peer_list)
			rate += p->getUploadRate();
		return rate;
	}

	QList<Peer*> PeerManager::getPeers() const
	{
		return d->peer_list;
	}


	//////////////////////////////////////////////////
	
	PeerManager::Private::Private(PeerManager* p, Torrent& tor)
		: p(p),tor(tor),available_chunks(tor.getNumChunks()),wanted_chunks(tor.getNumChunks())
	{
		started = false;
		wanted_chunks.setAll(true);
		wanted_changed = false;
		cnt = new ChunkCounter(tor.getNumChunks());
		pex_on = !tor.isPrivate();
		piece_handler = 0;
		paused = false;
		superseeder = 0;
	}

	PeerManager::Private::~Private()
	{
		delete cnt;
		ServerInterface::removePeerManager(p);
		
		if ((Uint32)peer_list.count() <= total_connections)
			total_connections -= peer_list.count();
		else
			total_connections = 0;
		
		qDeleteAll(peer_list.begin(),peer_list.end());
		peer_list.clear();
		
		delete superseeder;
		
		started = false;
		connectors.clear();
	}

	void PeerManager::Private::update()
	{
		if (!started)
			return;
		
		// update the speed of each peer,
		// and get rid of some killed peers
		QList<Peer*>::iterator i = peer_list.begin();
		while (i != peer_list.end())
		{
			Peer* peer = *i;
			if (!peer->isKilled() && peer->isStalled())
			{
				PotentialPeer pp;
				pp.port = peer->getPort();
				pp.local = peer->getStats().local;
				pp.ip = peer->getIPAddresss();
				peer->kill();
				p->addPotentialPeer(pp);
				Out(SYS_CON|LOG_NOTICE) << QString("Killed stalled peer %1").arg(pp.ip) << endl;
			}
			
			if (peer->isKilled())
			{
				cnt->decBitSet(peer->getBitSet());
				updateAvailableChunks();
				i = peer_list.erase(i);
				killed.append(peer);
				peer_map.erase(peer->getID());
				if (total_connections > 0)
					total_connections--;
				p->peerKilled(peer);
				if (superseeder)
					superseeder->peerRemoved(peer);
			}
			else
			{
				peer->update();
				i++;
			}
		}
		if (wanted_changed)
		{
			foreach (Peer* peer,peer_list)
			{
				if (peer->hasWantedChunks(wanted_chunks))
					peer->getPacketWriter().sendInterested();
				else
					peer->getPacketWriter().sendNotInterested();
				i++;
			}
			wanted_changed = false;
		}
	}

	void PeerManager::Private::have(Peer* peer, Uint32 index)
	{
		if (wanted_chunks.get(index) && !paused)
			peer->getPacketWriter().sendInterested();
		available_chunks.set(index,true);
		cnt->inc(index);
		if (superseeder)
			superseeder->have(peer,index);
	}
	
	void PeerManager::Private::updateAvailableChunks()
	{
		for (Uint32 i = 0;i < available_chunks.getNumBits();i++)
		{
			available_chunks.set(i,cnt->get(i) > 0);
		}
	}
	
	bool PeerManager::Private::killBadPeer()
	{
		for (PtrMap<Uint32,Peer>::iterator i = peer_map.begin();i != peer_map.end();i++)
		{
			Peer* peer = i->second;
			if (peer->getStats().aca_score <= -5.0 && peer->getStats().aca_score > -50.0)
			{
				Out(SYS_GEN|LOG_DEBUG) << "Killing bad peer, to make room for other peers" << endl;
				peer->kill();
				return true;
			}
		}
		return false;
	}
	
	void PeerManager::Private::createPeer(mse::StreamSocket::Ptr sock,const PeerID & peer_id,Uint32 support,bool local)
	{
		Peer* peer = new Peer(sock,peer_id,tor.getNumChunks(),tor.getChunkSize(),support,local,p);
		peer_list.append(peer);
		peer_map.insert(peer->getID(),peer);
		total_connections++;
		p->newPeer(peer);
		peer->setPexEnabled(pex_on);
		// send extension protocol handshake
		bt::Uint16 port = ServerInterface::getPort();
		peer->sendExtProtHandshake(port,tor.getMetaData().size());
		
		if (superseeder)
			superseeder->peerAdded(peer);
	}
	
	
	bool PeerManager::Private::connectedTo(const QString & ip,Uint16 port) const
	{
		PtrMap<Uint32,Peer>::const_iterator i = peer_map.begin();
		while (i != peer_map.end())
		{
			const Peer* peer = i->second;
			if (peer->getPort() == port && peer->getStats().ip_address == ip)
				return true;
			i++;
		}
		return false;
	}
	
	void PeerManager::Private::connectToPeers()
	{
		if (paused)
			return;
		
		if (potential_peers.size() == 0)
			return;
		
		if (peer_list.count() + connectors.size() >= max_connections && max_connections > 0)
			return;
		
		if (total_connections >= max_total_connections && max_total_connections > 0)
			return;
		
		if (connectors.size() > MAX_SIMULTANIOUS_AUTHS)
			return;
		
		Uint32 num = 0;
		if (max_connections > 0)
		{
			Uint32 available = max_connections - (peer_list.count() + connectors.size());
			num = available >= potential_peers.size() ? 
			potential_peers.size() : available;
		}
		else
		{
			num = potential_peers.size();
		}
		
		if (num + total_connections >= max_total_connections && max_total_connections > 0)
			num = max_total_connections - total_connections;
		
		for (Uint32 i = 0;i < num;i++)
		{
			if (connectors.size() > MAX_SIMULTANIOUS_AUTHS)
				return;
			
			PPItr itr = potential_peers.begin();
			
			AccessManager & aman = AccessManager::instance();
			
			if (aman.allowed(itr->first) && !connectedTo(itr->first,itr->second.port))
			{
				const PotentialPeer & pp = itr->second;
				PeerConnector::Ptr pcon(new PeerConnector(pp.ip,pp.port,pp.local,p));
				pcon->setWeakPointer(PeerConnector::WPtr(pcon));
				connectors.insert(pcon);
				total_connections++;
				pcon->start();
			}
			potential_peers.erase(itr);
		}
	}

}

#include "peermanager.moc"
