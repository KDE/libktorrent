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
#include "dht.h"
#include <QMap>
#include <net/addressresolver.h>
#include <util/log.h>
#include <util/array.h>
#include <util/error.h>
#include <util/functions.h>
#include <bcodec/bnode.h>
#include "announcetask.h"
#include "node.h"
#include "rpcserver.h"
#include "rpcmsg.h"
#include "kclosestnodessearch.h"
#include "database.h"
#include "taskmanager.h"
#include "nodelookup.h"
#include "pingreq.h"
#include "findnodereq.h"
#include "getpeersreq.h"
#include "announcereq.h"
#include "pingrsp.h"
#include "findnodersp.h"
#include "getpeersrsp.h"
#include "announcersp.h"

using namespace bt;

namespace dht
{



	DHT::DHT() : node(0), srv(0), db(0), tman(0), our_node_lookup(0)
	{
		connect(&update_timer, SIGNAL(timeout()), this, SLOT(update()));
		connect(&expire_timer, SIGNAL(timeout()), this, SLOT(expireDatabaseItems()));
	}


	DHT::~DHT()
	{
		if (running)
			stop();
	}

	void DHT::start(const QString & table, const QString & key_file, bt::Uint16 port)
	{
		if (running)
			return;

		if (port == 0)
			port = 6881;

		table_file = table;
		this->port = port;
		Out(SYS_DHT | LOG_NOTICE) << "DHT: Starting on port " << port << endl;
		srv = new RPCServer(this, port);
		node = new Node(srv, key_file);
		db = new Database();
		tman = new TaskManager(this);
		running = true;
		srv->start();
		node->loadTable(table);
		update_timer.start(1000);
		expire_timer.start(5*60*1000);
		started();
		if (node->getNumEntriesInRoutingTable() > 0)
		{
			// refresh the DHT table by looking for our own ID
			findOwnNode();
		}
	}


	void DHT::stop()
	{
		if (!running)
			return;

		update_timer.stop();
		expire_timer.stop();
		Out(SYS_DHT | LOG_NOTICE) << "DHT: Stopping " << endl;
		srv->stop();
		node->saveTable(table_file);
		running = false;
		stopped();
		delete tman; tman = 0;
		delete db; db = 0;
		delete node; node = 0;
		delete srv; srv = 0;
	}

	void DHT::ping(const PingReq & r)
	{
		if (!running)
			return;

		// ignore requests we get from ourself
		if (r.getID() == node->getOurID())
			return;

		PingRsp rsp(r.getMTID(), node->getOurID());
		rsp.setOrigin(r.getOrigin());
		srv->sendMsg(rsp);
		node->received(this, r);
	}

	void DHT::findNode(const dht::FindNodeReq& r)
	{
		if (!running)
			return;

		// ignore requests we get from ourself
		if (r.getID() == node->getOurID())
			return;

		node->received(this, r);
		// find the K closest nodes and pack them
		KClosestNodesSearch kns(r.getTarget(), K);

		bt::Uint32 wants = 0;
		if (r.wants(4) || r.getOrigin().ipVersion() == 4)
			wants |= WANT_IPV4;
		if (r.wants(6) || r.getOrigin().ipVersion() == 6)
			wants |= WANT_IPV6;

		node->findKClosestNodes(kns, wants);

		FindNodeRsp fnr(r.getMTID(), node->getOurID());
		// pack the found nodes in a byte array
		kns.pack(&fnr);
		fnr.setOrigin(r.getOrigin());
		srv->sendMsg(fnr);
	}

	NodeLookup* DHT::findOwnNode()
	{
		if (our_node_lookup)
			return our_node_lookup;

		our_node_lookup = findNode(node->getOurID());
		if (our_node_lookup)
			connect(our_node_lookup, SIGNAL(finished(Task*)), this, SLOT(ownNodeLookupFinished(Task*)));
		return our_node_lookup;
	}

	void DHT::ownNodeLookupFinished(Task* t)
	{
		if (our_node_lookup == t)
			our_node_lookup = 0;
	}


	void DHT::announce(const AnnounceReq & r)
	{
		if (!running)
			return;

		// ignore requests we get from ourself
		if (r.getID() == node->getOurID())
			return;

		node->received(this, r);
		// first check if the token is OK
		dht::Key token = r.getToken();
		if (!db->checkToken(token, r.getOrigin()))
			return;

		// everything OK, so store the value
		db->store(r.getInfoHash(), DBItem(r.getOrigin()));
		// send a proper response to indicate everything is OK
		AnnounceRsp rsp(r.getMTID(), node->getOurID());
		rsp.setOrigin(r.getOrigin());
		srv->sendMsg(rsp);
	}



	void DHT::getPeers(const GetPeersReq & r)
	{
		if (!running)
			return;

		// ignore requests we get from ourself
		if (r.getID() == node->getOurID())
			return;

		node->received(this, r);
		DBItemList dbl;
		db->sample(r.getInfoHash(), dbl, 50, r.getOrigin().ipVersion());

		// generate a token
		dht::Key token = db->genToken(r.getOrigin());

		bt::Uint32 wants = 0;
		if (r.wants(4) || r.getOrigin().ipVersion() == 4)
			wants |= WANT_IPV4;
		if (r.wants(6) || r.getOrigin().ipVersion() == 6)
			wants |= WANT_IPV6;

		// if data is null do the same as when we have a findNode request
		// find the K closest nodes and pack them
		KClosestNodesSearch kns(r.getInfoHash(), K);
		node->findKClosestNodes(kns, wants);

		GetPeersRsp fnr(r.getMTID(), node->getOurID(), dbl, token);
		kns.pack(&fnr);
		fnr.setOrigin(r.getOrigin());
		srv->sendMsg(fnr);
	}

	void DHT::response(const RPCMsg & r)
	{
		if (!running)
			return;

		node->received(this, r);
	}

	void DHT::error(const ErrMsg & msg)
	{
		Q_UNUSED(msg);
	}


	void DHT::portReceived(const QString & ip, bt::Uint16 port)
	{
		if (!running)
			return;

		RPCMsg::Ptr r(new PingReq(node->getOurID()));
		r->setOrigin(net::Address(ip, port));
		srv->doCall(r);
	}

	bool DHT::canStartTask() const
	{
		// we can start a task if we have less then  7 runnning and
		// there are at least 16 RPC slots available
		if (tman->getNumTasks() >= 7)
			return false;
		else if (256 - srv->getNumActiveRPCCalls() <= 16)
			return false;

		return true;
	}

	AnnounceTask* DHT::announce(const bt::SHA1Hash & info_hash, bt::Uint16 port)
	{
		if (!running)
			return 0;

		KClosestNodesSearch kns(info_hash, K);
		node->findKClosestNodes(kns, WANT_BOTH);
		if (kns.getNumEntries() > 0)
		{
			Out(SYS_DHT | LOG_NOTICE) << "DHT: Doing announce " << endl;
			AnnounceTask* at = new AnnounceTask(db, srv, node, info_hash, port, tman);
			at->start(kns, !canStartTask());
			tman->addTask(at);
			if (!db->contains(info_hash))
				db->insert(info_hash);
			return at;
		}

		return 0;
	}

	NodeLookup* DHT::refreshBucket(const dht::Key & id, KBucket & bucket)
	{
		if (!running)
			return 0;

		KClosestNodesSearch kns(id, K);
		bucket.findKClosestNodes(kns);
		bucket.updateRefreshTimer();
		if (kns.getNumEntries() > 0)
		{
			Out(SYS_DHT | LOG_DEBUG) << "DHT: refreshing bucket " << endl;
			NodeLookup* nl = new NodeLookup(id, srv, node, tman);
			nl->start(kns, !canStartTask());
			tman->addTask(nl);
			return nl;
		}

		return 0;
	}

	NodeLookup* DHT::findNode(const dht::Key & id)
	{
		if (!running)
			return 0;

		KClosestNodesSearch kns(id, K);
		node->findKClosestNodes(kns, WANT_BOTH);
		if (kns.getNumEntries() > 0)
		{
			Out(SYS_DHT | LOG_DEBUG) << "DHT: finding node " << endl;
			NodeLookup* at = new NodeLookup(id, srv, node, tman);
			at->start(kns, !canStartTask());
			tman->addTask(at);
			return at;
		}

		return 0;
	}

	void DHT::expireDatabaseItems()
	{
		db->expire(bt::CurrentTime());
	}


	void DHT::update()
	{
		if (!running)
			return;

		try
		{
			node->refreshBuckets(this);
			stats.num_tasks = tman->getNumTasks() + tman->getNumQueuedTasks();
			stats.num_peers = node->getNumEntriesInRoutingTable();
		}
		catch (bt::Error & e)
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Error: " << e.toString() << endl;
		}
	}

	void DHT::timeout(RPCMsg::Ptr r)
	{
		node->onTimeout(r);
	}

	void DHT::addDHTNode(const QString & host, Uint16 hport)
	{
		if (!running)
			return;

		net::Address addr;
		if (addr.setAddress(host))
		{
			addr.setPort(hport);
			srv->ping(node->getOurID(), addr);
		}
		else
			net::AddressResolver::resolve(host, hport, this, SLOT(hostResolved(net::AddressResolver*)));
	}

	void DHT::onResolverResults(net::AddressResolver* res)
	{
		if (!running)
			return;

		if (res->succeeded())
		{
			srv->ping(node->getOurID(), res->address());
		}
	}
	
	QMap<QString, int> DHT::getClosestGoodNodes(int maxNodes)
	{
		QMap<QString, int> map;
		
		if (!node)
			return map;
		
		int max = 0;
		KClosestNodesSearch kns(node->getOurID(), maxNodes*2);
		node->findKClosestNodes(kns, WANT_BOTH);
		
		KClosestNodesSearch::Itr it;
		for (it = kns.begin(); it != kns.end(); ++it)
		{
			KBucketEntry e = it->second;
			
			if (!e.isGood())
				continue;
			
			const net::Address & a = e.getAddress();
			
			map.insert(a.toString(), a.port());
			if (++max >= maxNodes)
				break;
		}
		
		return map;
	}
	
}
