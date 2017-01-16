/***************************************************************************
 *   Copyright (C) 2012 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
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
		connect(&update_timer, &QTimer::timeout, this, &DHT::update);
		connect(&expire_timer, &QTimer::timeout, this, &DHT::expireDatabaseItems);
		/**
		 * @author Fonic <https://github.com/fonic>
		 * Connect timer for bootstrap check.
		 */
		connect(&bootstrap_timer, &QTimer::timeout, this, &DHT::checkBootstrap);
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
		/**
		 * @author Fonic <https://github.com/fonic>
		 * Use constant defined in dht.h instead of hard-coded value.
		 */
		expire_timer.start(DATABASE_EXPIRE_INTERVAL);
		started();
		if (node->getNumEntriesInRoutingTable() > 0)
		{
			// Refresh the DHT table by looking for our own ID
			findOwnNode();
		}
		else
		{
			/**
			* @author Fonic <https://github.com/fonic>
			* Routing table is empty, bootstrap using well-known nodes. Start
			* timer to check result and retry if necessary.
			*/
			Out(SYS_DHT | LOG_NOTICE) << "DHT: Routing table empty, bootstrapping from well-known nodes" << endl;
			bootstrap();
			bootstrap_retries = 0;
			bootstrap_timer.start(BOOTSTRAP_CHECK_INTERVAL);
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

		// Ignore requests we get from ourself
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

		// Ignore requests we get from ourself
		if (r.getID() == node->getOurID())
			return;

		node->received(this, r);
		// Find the K closest nodes and pack them
		KClosestNodesSearch kns(r.getTarget(), K);

		bt::Uint32 wants = 0;
		if (r.wants(4) || r.getOrigin().ipVersion() == 4)
			wants |= WANT_IPV4;
		if (r.wants(6) || r.getOrigin().ipVersion() == 6)
			wants |= WANT_IPV6;

		node->findKClosestNodes(kns, wants);

		FindNodeRsp fnr(r.getMTID(), node->getOurID());
		// Pack the found nodes in a byte array
		kns.pack(&fnr);
		fnr.setOrigin(r.getOrigin());
		srv->sendMsg(fnr);
	}

	NodeLookup* DHT::findOwnNode()
	{
		if (our_node_lookup)
			return our_node_lookup;

		/**
		 * @author Fonic <https://github.com/fonic>
		 * Added additional log debug output.
		 */
		Out(SYS_DHT | LOG_DEBUG) << "DHT: Finding own node" << endl;
		our_node_lookup = findNode(node->getOurID());
		if (our_node_lookup)
			connect(our_node_lookup, &NodeLookup::finished, this, &DHT::ownNodeLookupFinished);
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

		// Ignore requests we get from ourself
		if (r.getID() == node->getOurID())
			return;

		node->received(this, r);
		// First check if the token is OK
		dht::Key token = r.getToken();
		if (!db->checkToken(token, r.getOrigin()))
			return;

		// Everything OK, so store the value
		db->store(r.getInfoHash(), DBItem(r.getOrigin()));
		// Send a proper response to indicate everything is OK
		AnnounceRsp rsp(r.getMTID(), node->getOurID());
		rsp.setOrigin(r.getOrigin());
		srv->sendMsg(rsp);
	}

	void DHT::getPeers(const GetPeersReq & r)
	{
		if (!running)
			return;

		// Ignore requests we get from ourself
		if (r.getID() == node->getOurID())
			return;

		node->received(this, r);
		DBItemList dbl;
		db->sample(r.getInfoHash(), dbl, 50, r.getOrigin().ipVersion());

		// Generate a token
		dht::Key token = db->genToken(r.getOrigin());

		bt::Uint32 wants = 0;
		if (r.wants(4) || r.getOrigin().ipVersion() == 4)
			wants |= WANT_IPV4;
		if (r.wants(6) || r.getOrigin().ipVersion() == 6)
			wants |= WANT_IPV6;

		// If data is null do the same as when we have a findNode request
		// Find the K closest nodes and pack them
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
		/**
		 * @author Fonic <https://github.com/fonic>
		 * A task may only be started if the amount of already running task
		 * is below the defined limit and if there is still a minimum amount
		 * of RPC slots available. Both values used to be hard-coded and are
		 * now defined by constants in dht.h.
		 */
		if (tman->getNumTasks() >= MAX_CONCURRENT_TASKS)
			return false;
		else if (256 - srv->getNumActiveRPCCalls() <= MIN_AVAILABLE_RPC_SLOTS)
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
			Out(SYS_DHT | LOG_DEBUG) << "DHT: Refreshing bucket " << endl;
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
			Out(SYS_DHT | LOG_DEBUG) << "DHT: Finding node " << endl;
			NodeLookup* at = new NodeLookup(id, srv, node, tman);
			at->start(kns, !canStartTask());
			tman->addTask(at);
			return at;
		}

		return 0;
	}

	void DHT::expireDatabaseItems()
	{
		/**
		 * @author Fonic <https://github.com/fonic>
		 * Added additional log debug output.
		 */
		Out(SYS_DHT | LOG_DEBUG) << "DHT: Expiring database items" << endl;
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

	/**
	 * @author Fonic <https://github.com/fonic>
	 * This method already existed, but seemed to be unused. It would have been
	 * unusable anyway due to referencing the non-existing slot 'hostResolved'.
	 * Reference fixed, method is now used for DHT bootstrapping.
	 */
	void DHT::addDHTNode(const QString & host, Uint16 hport)
	{
		if (!running)
			return;

		Out(SYS_DHT | LOG_DEBUG) << "DHT: Adding bootstrap node '" << host << ":" << hport << "'" << endl;
		net::Address addr;
		if (addr.setAddress(host))
		{
			addr.setPort(hport);
			srv->ping(node->getOurID(), addr);
		}
		else
		{
			Out(SYS_DHT | LOG_DEBUG) << "DHT: Resolving bootstrap node '" << host << "'" << endl;
			net::AddressResolver::resolve(host, hport, this, SLOT(onResolverResults(net::AddressResolver*)));
		}
	}

	void DHT::onResolverResults(net::AddressResolver* res)
	{
		if (!running)
			return;

		if (res->succeeded())
		{
			/**
			 * @author Fonic <https://github.com/fonic>
			 * Added additional log debug output.
			 */
			Out(SYS_DHT | LOG_DEBUG) << "DHT: Adding resolved bootstrap node '" << res->address().toString() << ":" << res->address().port() << "'" << endl;
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

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Bootstrap DHT from well-knows nodes. The list of existing well-known
	 * nodes was compiled through an extensive online research. For now, hard-
	 * coded entries are used, as most other torrent clients do. This should
	 * probably be made user-configurable as an advanced setting.
	 *
	 * Identified well-known nodes as of 11/08/16:
	 * router.bittorrent.com:6881  - works reliably
	 * router.utorrent.com:6881    - works reliably
	 * dht.libtorrent.org:25401    - works reliably
	 * dht.transmissionbt.com:6881 - works most of the time
	 * dht.aelitis.com:6881        - not working, supposedly different protocol
	 * router.bitcomet.com:6881    - DNS error (unknown host)
	 * router.bitcomet.net:554     - DNS error (resolves to 127.0.0.1)
	 */
	void DHT::bootstrap()
	{
		Out(SYS_DHT | LOG_DEBUG) << "DHT: Adding well-known bootstrapping nodes" << endl;
		addDHTNode(QString("router.bittorrent.com"), 6881);
		addDHTNode(QString("router.utorrent.com"), 6881);
		addDHTNode(QString("dht.libtorrent.org"), 25401);
		//addDHTNode(QString("dht.transmissionbt.com"), 6881);
		//addDHTNode(QString("dht.aelitis.com"), 6881);
		//addDHTNode(QString("router.bitcomet.com"), 6881);
		//addDHTNode(QString("router.bitcomet.net"), 554);
	}

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Check bootstrap result, retry if necessary. This method is used as the
	 * timeout handler for bootstrap_timer. Since there is no easy way to tell
	 * wether bootstrapping was sucessful, the number of entries in the routing
	 * table is compared to a predefined minimum value. Retries are counted,
	 * retrying stops when the predefined limit is reached.
	 */
	void DHT::checkBootstrap()
	{
		bootstrap_retries++;

		bt::Uint32 num_entries = node->getNumEntriesInRoutingTable();
		if (num_entries >= BOOTSTRAP_MIN_ENTRIES)
		{
			//Out(SYS_DHT | LOG_NOTICE) << "DHT: Bootstrapping successful, routing table contains " << num_entries << " entries (required: " << BOOTSTRAP_MIN_ENTRIES << ")" << endl;
			Out(SYS_DHT | LOG_NOTICE) << "DHT: Bootstrapping successful" << endl;
			bootstrap_timer.stop();
		}
		else
		{
			if (bootstrap_retries < BOOTSTRAP_MAX_RETRIES)
			{
				//Out(SYS_DHT | LOG_NOTICE) << "DHT: Bootstrapping failed, routing table contains " << num_entries << " entries (required: " << BOOTSTRAP_MIN_ENTRIES << "), retrying" << endl;
				Out(SYS_DHT | LOG_NOTICE) << "DHT: Bootstrapping failed, retrying" << endl;
				bootstrap();
			}
			else
			{
				//Out(SYS_DHT | LOG_NOTICE) << "DHT: Bootstrapping failed, maximum number of retries reached (limit: " << BOOTSTRAP_MAX_RETRIES << ")" << endl;
				Out(SYS_DHT | LOG_NOTICE) << "DHT: Bootstrapping failed, maximum number of retries reached" << endl;
				bootstrap_timer.stop();
			}
		}
	}

}
