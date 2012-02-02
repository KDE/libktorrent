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
#include <mse/encryptedpacketsocket.h>
#include <mse/encryptedauthenticate.h>
#include <peer/accessmanager.h>
#include <torrent/globals.h>
#include <torrent/server.h>
#include <dht/dhtbase.h>
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

	typedef std::map<net::Address, bool>::iterator PPItr;
	typedef QMap<Uint32, Peer::Ptr> PeerMap;


	class PeerManager::Private
	{
	public:
		Private(PeerManager* p, Torrent & tor);
		~Private();

		void updateAvailableChunks();
		bool killBadPeer();
		void createPeer(mse::EncryptedPacketSocket::Ptr sock, const PeerID & peer_id, Uint32 support, bool local);
		bool connectedTo(const net::Address & addr) const;
		void update();
		void have(Peer* peer, Uint32 index);
		void connectToPeers();

	public:
		PeerManager* p;
		QMap<Uint32, Peer::Ptr> peer_map;
		Torrent & tor;
		bool started;
		BitSet available_chunks, wanted_chunks;
		ChunkCounter cnt;
		bool pex_on;
		bool wanted_changed;
		PieceHandler* piece_handler;
		bool paused;
		QSet<PeerConnector::Ptr> connectors;
		QScopedPointer<SuperSeeder> superseeder;
		std::map<net::Address, bool> potential_peers;
		bool partial_seed;
		Uint32 num_cleared;
	};

	PeerManager::PeerManager(Torrent & tor)
			: d(new Private(this, tor))
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

		foreach(Peer::Ptr p, d->peer_map)
		{
			p->pause();
		}
		d->paused = true;
	}

	void PeerManager::unpause()
	{
		if (!d->paused)
			return;

		foreach(Peer::Ptr p, d->peer_map)
		{
			p->unpause();
			if (p->hasWantedChunks(d->wanted_chunks)) // send interested when it has wanted chunks
				p->sendInterested();
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

	void PeerManager::addPotentialPeer(const net::Address & addr, bool local)
	{
		if (d->potential_peers.size() < 500)
		{
			d->potential_peers[addr] = local;
		}
	}

	void PeerManager::killSeeders()
	{
		foreach(Peer::Ptr peer, d->peer_map)
		{
			if (peer->isSeeder())
				peer->kill();
		}
	}

	void PeerManager::killUninterested()
	{
		QTime now = QTime::currentTime();
		foreach(Peer::Ptr peer, d->peer_map)
		{
			if (!peer->isInterested() && (peer->getConnectTime().secsTo(now) > 30))
				peer->kill();
		}
	}

	void PeerManager::have(Peer* p, Uint32 index)
	{
		d->have(p, index);
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
				d->available_chunks.set(i, true);
				d->cnt.inc(i);
			}
		}

		if (interested && !d->paused)
			p->sendInterested();

		if (d->superseeder)
			d->superseeder->bitset(p, bs);
	}


	void PeerManager::newConnection(mse::EncryptedPacketSocket::Ptr sock, const PeerID & peer_id, Uint32 support)
	{
		if (!d->started)
			return;

		Uint32 total = d->peer_map.count() + d->connectors.size();
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

		d->createPeer(sock, peer_id, support, false);
	}

	void PeerManager::peerAuthenticated(Authenticate* auth, PeerConnector::WPtr pcon, bool ok)
	{
		if (d->started)
		{
			if (total_connections > 0)
				total_connections--;

			if (ok && !connectedTo(auth->getPeerID()))
				d->createPeer(auth->getSocket(), auth->getPeerID(), auth->supportedExtensions(), auth->isLocal());
		}

		PeerConnector::Ptr ptr = pcon.toStrongRef();
		d->connectors.remove(ptr);
	}



	bool PeerManager::connectedTo(const PeerID & peer_id)
	{
		if (!d->started)
			return false;

		foreach(Peer::Ptr p, d->peer_map)
		{
			if (p->getPeerID() == peer_id)
				return true;
		}
		return false;
	}

	void PeerManager::closeAllConnections()
	{
		if ((Uint32)d->peer_map.count() <= total_connections)
			total_connections -= d->peer_map.count();
		else
			total_connections = 0;

		d->peer_map.clear();
	}


	void PeerManager::savePeerList(const QString & file)
	{
		// Lets save the entries line based
		QFile fptr(file);
		if (!fptr.open(QIODevice::WriteOnly))
			return;

		try
		{
			Out(SYS_GEN | LOG_DEBUG) << "Saving list of peers to " << file << endl;

			QTextStream out(&fptr);
			// first the active peers
			foreach(Peer::Ptr p, d->peer_map)
			{
				const net::Address & addr = p->getAddress();
				out << addr.toString() << " " << (unsigned short)addr.port() << ::endl;
			}

			// now the potential_peers
			PPItr i = d->potential_peers.begin();
			while (i != d->potential_peers.end())
			{
				out << i->first.toString() << " " <<  i->first.port() << ::endl;
				i++;
			}
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN | LOG_DEBUG) << "Error happened during saving of peer list : " << err.toString() << endl;
		}
	}

	void PeerManager::loadPeerList(const QString & file)
	{
		QFile fptr(file);
		if (!fptr.open(QIODevice::ReadOnly))
			return;

		try
		{

			Out(SYS_GEN | LOG_DEBUG) << "Loading list of peers from " << file  << endl;

			while (!fptr.atEnd())
			{
				QStringList sl = QString(fptr.readLine()).split(" ");
				if (sl.count() != 2)
					continue;

				bool ok = false;
				net::Address addr(sl[0], sl[1].toInt(&ok));
				if (ok)
					addPotentialPeer(addr, false);
			}
		}
		catch (bt::Error & err)
		{
			Out(SYS_GEN | LOG_DEBUG) << "Error happened during saving of peer list : " << err.toString() << endl;
		}
	}

	void PeerManager::start(bool superseed)
	{
		d->started = true;
		if (superseed && !d->superseeder)
			d->superseeder.reset(new SuperSeeder(d->cnt.getNumChunks()));

		unpause();
		ServerInterface::addPeerManager(this);
	}


	void PeerManager::stop()
	{
		d->cnt.reset();
		d->available_chunks.clear();
		d->started = false;
		ServerInterface::removePeerManager(this);
		d->connectors.clear();
		d->superseeder.reset();
		closeAllConnections();
	}

	Peer::Ptr PeerManager::findPeer(Uint32 peer_id)
	{
		PeerMap::iterator i = d->peer_map.find(peer_id);
		if (i == d->peer_map.end())
			return Peer::Ptr();
		else
			return *i;
	}

	Peer::Ptr PeerManager::findPeer(PieceDownloader* pd)
	{
		foreach(Peer::Ptr p, d->peer_map)
		{
			if ((PieceDownloader*)p->getPeerDownloader() == pd)
				return p;
		}
		return Peer::Ptr();
	}

	void PeerManager::rerunChoker()
	{
		d->num_cleared++;
	}
	
	bool PeerManager::chokerNeedsToRun() const
	{
		return d->num_cleared > 0;
	}

	void PeerManager::peerSourceReady(PeerSource* ps)
	{
		net::Address addr;
		bool local = false;
		while (ps->takePeer(addr, local))
			addPotentialPeer(addr, local);
	}



	void PeerManager::pex(const QByteArray & arr)
	{
		if (!d->pex_on)
			return;

		Out(SYS_CON | LOG_NOTICE) << "PEX: found " << (arr.size() / 6) << " peers"  << endl;
		for (int i = 0;i + 6 <= arr.size();i += 6)
		{
			const Uint8* tmp = (const Uint8*)arr.data() + i;
			addPotentialPeer(net::Address(ReadUint32(tmp, 0), ReadUint16(tmp, 4)), false);
		}
	}


	void PeerManager::setPexEnabled(bool on)
	{
		if (on && d->tor.isPrivate())
			return;

		if (d->pex_on == on)
			return;

		foreach(Peer::Ptr p, d->peer_map)
		{
			if (!p->isKilled())
			{
				p->setPexEnabled(on);
				bt::Uint16 port = ServerInterface::getPort();
				p->sendExtProtHandshake(port, d->tor.getMetaData().size(), d->partial_seed);
			}
		}
		d->pex_on = on;
	}

	void PeerManager::setGroupIDs(Uint32 up, Uint32 down)
	{
		for (PeerMap::iterator i = d->peer_map.begin();i != d->peer_map.end();i++)
			i.value()->setGroupIDs(up, down);
	}

	void PeerManager::portPacketReceived(const QString& ip, Uint16 port)
	{
		if (Globals::instance().getDHT().isRunning() && !d->tor.isPrivate())
			Globals::instance().getDHT().portReceived(ip, port);
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
		foreach(Peer::Ptr p, d->peer_map)
		{
			if (p->getDownloadRate() == 0 && p->getUploadRate() == 0)
				p->kill();
		}
	}

	void PeerManager::setSuperSeeding(bool on, const BitSet & chunks)
	{
		Q_UNUSED(chunks);
		if ((d->superseeder && on) || (!d->superseeder && !on))
			return;

		d->superseeder.reset(on ? new SuperSeeder(d->cnt.getNumChunks()) : 0);
		
		// When entering or exiting superseeding mode kill all peers
		// but first add the current list to the potential_peers list, so we can reconnect later.
		foreach(Peer::Ptr p, d->peer_map)
		{
			const net::Address & addr = p->getAddress();
			addPotentialPeer(addr, false);
			p->kill();
		}
	}

	void PeerManager::sendHave(Uint32 index)
	{
		if (d->superseeder)
			return;

		foreach(Peer::Ptr peer, d->peer_map)
		{
			peer->sendHave(index);
		}
	}

	Uint32 PeerManager::getNumConnectedPeers() const
	{
		return d->peer_map.count();
	}
	
	Uint32 PeerManager::getNumConnectedLeechers() const
	{
		Uint32 cnt = 0;
		foreach(Peer::Ptr peer, d->peer_map)
		{
			if (!peer->isSeeder())
				cnt++;
		}
		
		return cnt;
	}

	Uint32 PeerManager::getNumConnectedSeeders() const
	{
		Uint32 cnt = 0;
		foreach(Peer::Ptr peer, d->peer_map)
		{
			if (peer->isSeeder())
				cnt++;
		}
		
		return cnt;
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
		return d->cnt;
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
		foreach(const Peer::Ptr p, d->peer_map)
		{
			visitor.visit(p);
		}
	}

	Uint32 PeerManager::uploadRate() const
	{
		Uint32 rate = 0;
		foreach(const Peer::Ptr p, d->peer_map)
		{
			rate += p->getUploadRate();
		}
		return rate;
	}

	QList<Peer::Ptr> PeerManager::getPeers() const
	{
		return d->peer_map.values();
	}

	void PeerManager::setPartialSeed(bool partial_seed)
	{
		if (d->partial_seed != partial_seed)
		{
			d->partial_seed = partial_seed;

			// If partial seeding status changes, update all peers
			bt::Uint16 port = ServerInterface::getPort();
			foreach(Peer::Ptr peer, d->peer_map)
			{
				peer->sendExtProtHandshake(port, d->tor.getMetaData().size(), d->partial_seed);
			}
		}
	}

	bool PeerManager::isPartialSeed() const
	{
		return d->partial_seed;
	}


	//////////////////////////////////////////////////

	PeerManager::Private::Private(PeerManager* p, Torrent& tor)
			: p(p),
			tor(tor),
			available_chunks(tor.getNumChunks()),
			wanted_chunks(tor.getNumChunks()),
			cnt(tor.getNumChunks()),
			partial_seed(false),
			num_cleared(0)
	{
		started = false;
		wanted_chunks.setAll(true);
		wanted_changed = false;
		pex_on = !tor.isPrivate();
		piece_handler = 0;
		paused = false;
	}

	PeerManager::Private::~Private()
	{
		ServerInterface::removePeerManager(p);

		if ((Uint32)peer_map.count() <= total_connections)
			total_connections -= peer_map.count();
		else
			total_connections = 0;

		started = false;
		connectors.clear();
	}

	void PeerManager::Private::update()
	{
		if (!started)
			return;

		num_cleared = 0;
		
		// update the speed of each peer,
		// and get rid of some killed peers
		for (PeerMap::iterator i = peer_map.begin(); i != peer_map.end();)
		{
			Peer::Ptr peer = i.value();
			if (!peer->isKilled() && peer->isStalled())
			{
				p->addPotentialPeer(peer->getAddress(), peer->getStats().local);
				peer->kill();
			}

			if (peer->isKilled())
			{
				cnt.decBitSet(peer->getBitSet());
				updateAvailableChunks();
				i = peer_map.erase(i);
				if (total_connections > 0)
					total_connections--;
				p->peerKilled(peer.data());
				if (superseeder)
					superseeder->peerRemoved(peer.data());
				num_cleared++;
			}
			else
			{
				peer->update();
				if (wanted_changed)
				{
					if (peer->hasWantedChunks(wanted_chunks))
						peer->sendInterested();
					else
						peer->sendNotInterested();
				}
				i++;
			}
		}

		wanted_changed = false;
		connectToPeers();
	}

	void PeerManager::Private::have(Peer* peer, Uint32 index)
	{
		if (wanted_chunks.get(index) && !paused)
			peer->sendInterested();
		available_chunks.set(index, true);
		cnt.inc(index);
		if (superseeder)
			superseeder->have(peer, index);
	}

	void PeerManager::Private::updateAvailableChunks()
	{
		for (Uint32 i = 0;i < available_chunks.getNumBits();i++)
		{
			available_chunks.set(i, cnt.get(i) > 0);
		}
	}

	bool PeerManager::Private::killBadPeer()
	{
		for (PeerMap::iterator i = peer_map.begin();i != peer_map.end();i++)
		{
			Peer::Ptr peer = i.value();
			if (peer->getStats().aca_score <= -5.0 && peer->getStats().aca_score > -50.0)
			{
				Out(SYS_GEN | LOG_DEBUG) << "Killing bad peer, to make room for other peers" << endl;
				peer->kill();
				return true;
			}
		}
		return false;
	}

	void PeerManager::Private::createPeer(mse::EncryptedPacketSocket::Ptr sock, const PeerID & peer_id, Uint32 support, bool local)
	{
		Peer::Ptr peer(new Peer(sock, peer_id, tor.getNumChunks(), tor.getChunkSize(), support, local, p));
		peer_map.insert(peer->getID(), peer);
		total_connections++;
		p->newPeer(peer.data());
		peer->setPexEnabled(pex_on);
		// send extension protocol handshake
		bt::Uint16 port = ServerInterface::getPort();
		peer->sendExtProtHandshake(port, tor.getMetaData().size(), partial_seed);

		if (superseeder)
			superseeder->peerAdded(peer.data());
	}


	bool PeerManager::Private::connectedTo(const net::Address & addr) const
	{
		PeerMap::const_iterator i = peer_map.begin();
		while (i != peer_map.end())
		{
			if (i.value()->getAddress() == addr)
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

		if (peer_map.count() + connectors.size() >= (int)max_connections && max_connections > 0)
			return;

		if (total_connections >= max_total_connections && max_total_connections > 0)
			return;

		if ((Uint32)connectors.size() > MAX_SIMULTANIOUS_AUTHS)
			return;

		Uint32 num = 0;
		if (max_connections > 0)
		{
			Uint32 available = max_connections - (peer_map.count() + connectors.size());
			num = qMin<Uint32>(available, potential_peers.size());
		}
		else
		{
			num = potential_peers.size();
		}

		if (num + total_connections >= max_total_connections && max_total_connections > 0)
			num = max_total_connections - total_connections;

		for (Uint32 i = 0;i < num && !potential_peers.empty();i++)
		{
			if ((Uint32)connectors.size() > MAX_SIMULTANIOUS_AUTHS)
				return;

			PPItr itr = potential_peers.begin();

			AccessManager & aman = AccessManager::instance();

			if (aman.allowed(itr->first) && !connectedTo(itr->first))
			{
				PeerConnector::Ptr pcon(new PeerConnector(itr->first, itr->second, p));
				pcon->setWeakPointer(PeerConnector::WPtr(pcon));
				connectors.insert(pcon);
				total_connections++;
				pcon->start();
			}
			potential_peers.erase(itr);
		}
	}

}
