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
#ifndef DHTDHT_H
#define DHTDHT_H

#include <qtimer.h>
#include <qstring.h>
#include <qmap.h>
#include <util/constants.h>
#include <util/timer.h>
#include "key.h"
#include "dhtbase.h"
#include "rpcmsg.h"

namespace net
{
	class AddressResolver;
}

namespace bt
{
	class SHA1Hash;
}

namespace dht
{
	class Node;
	class RPCServer;
	class Database;
	class TaskManager;
	class Task;
	class AnnounceTask;
	class NodeLookup;
	class KBucket;
	class ErrMsg;
	class PingReq;
	class FindNodeReq;
	class GetPeersReq;
	class AnnounceReq;

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Interval used for expiring database items (i.e. checking the database for
	 * expired items). This is not to be confused with MAX_ITEM_AGE in database.h
	 * which defines _when_ database items expire). A value of 5 mins is proposed
	 * by the reference implementation. This should probably be made user-config-
	 * urable as an advanced setting.
	 */
	const bt::Uint32 DATABASE_EXPIRE_INTERVAL = 5 * 60 * 1000;

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Interval used for checking bootstrap result and retrying if necessary. One
	 * minute should be enough time to complete bootstrapping. This should probably
	 * be made user-configurable as an advanced setting.
	 */
	const bt::Uint32 BOOTSTRAP_CHECK_INTERVAL = 1 * 60 * 1000;

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Maximum number of bootstrapping retries. This should probably be made user-
	 * configurable as an advanced setting.
	 */
	const bt::Uint32 BOOTSTRAP_MAX_RETRIES = 3;

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Minimum amount of routing table entries bootstrapping has to yield in order
	 * to be considered sucessful. This should probably be made user-configurable
	 * as an advanced setting.
	 */
	const bt::Uint32 BOOTSTRAP_MIN_ENTRIES = 3;

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Maximum number of concurrent tasks (i.e. outstanding DHT requests). This
	 * used to be hard-coded in dht.cpp to a value of 7. Comparison to values in
	 * the reference implementation is difficult, as the RI uses a finer-grained
	 * mechanism (https://github.com/bittorrent/libbtdht/blob/master/src/DhtImpl.h,
	 * @line 806ff, enum 'KademliaConstants'). This should probably be made user-
	 * configurable as an advanced setting.
	 */
	const bt::Uint32 MAX_CONCURRENT_TASKS = 10;

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Minimum amount of RPC slots that have to be available to start a new task.
	 * This used to be hard-coded in dht.cpp to a value of 16. This value seems
	 * to have an internal-only scope and should not be made user-configurable.
	 */
	const bt::Uint32 MIN_AVAILABLE_RPC_SLOTS = 16;

	/**
	 * @author Joris Guisson <joris.guisson@gmail.com>
	 */
	class DHT : public DHTBase
	{
		Q_OBJECT
	public:
		DHT();
		virtual ~DHT();

		void ping(const PingReq & r);
		void findNode(const FindNodeReq & r);
		void response(const RPCMsg & r);
		void getPeers(const GetPeersReq &  r);
		void announce(const AnnounceReq & r);
		void error(const ErrMsg & r);
		void timeout(RPCMsg::Ptr r);

		/**
		 * A Peer has received a PORT message, and uses this function to alert the DHT of it.
		 * @param ip The IP of the peer
		 * @param port The port in the PORT message
		 */
		void portReceived(const QString & ip,bt::Uint16 port);

		/**
		 * Do an announce on the DHT network
		 * @param info_hash The info_hash
		 * @param port The port
		 * @return The task which handles this
		 */
		AnnounceTask* announce(const bt::SHA1Hash & info_hash,bt::Uint16 port);

		/**
		 * Refresh a bucket using a find node task.
		 * @param id The id
		 * @param bucket The bucket to refresh
		 */
		NodeLookup* refreshBucket(const dht::Key & id,KBucket & bucket);

		/**
		 * Do a NodeLookup.
		 * @param id The id of the key to search
		 */
		NodeLookup* findNode(const dht::Key & id);

		/**
		 * Do a findNode for our node id
		 */
		NodeLookup* findOwnNode();

		/**
		 * See if it is possible to start a task
		 */
		bool canStartTask() const;

		void start(const QString & table,const QString & key_file,bt::Uint16 port);
		void stop();
		void addDHTNode(const QString & host,bt::Uint16 hport);

		virtual QMap<QString, int> getClosestGoodNodes(int maxNodes);

		/**
		 * @author Fonic <https://github.com/fonic>
		 * Bootstrap from well-known nodes.
		 */
		void bootstrap();

	private slots:
		void update();
		void onResolverResults(net::AddressResolver* ar);
		void ownNodeLookupFinished(Task* t);
		void expireDatabaseItems();
		/**
		 * @author Fonic <https://github.com/fonic>
		 * Timeout handler for bootstrap timer.
		 */
		void checkBootstrap();

	private:
		Node* node;
		RPCServer* srv;
		Database* db;
		TaskManager* tman;
		QTimer expire_timer;
		QString table_file;
		QTimer update_timer;
		NodeLookup* our_node_lookup;
		/**
		 * @author Fonic <https://github.com/fonic>
		 * Timer for bootstrap result check, retry counter.
		 */
		QTimer bootstrap_timer;
		bt::Uint32 bootstrap_retries;
	};

}

#endif // DHTDHT_H
