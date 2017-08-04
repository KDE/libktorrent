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
		@author Joris Guisson <joris.guisson@gmail.com>
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
		
		/// Do a findNode for our node id
		NodeLookup* findOwnNode();
		
		/// See if it is possible to start a task
		bool canStartTask() const;
		
		void start(const QString & table,const QString & key_file,bt::Uint16 port);
		void stop();
		void addDHTNode(const QString & host,bt::Uint16 hport);
		
		virtual QMap<QString, int> getClosestGoodNodes(int maxNodes);
		
	private Q_SLOTS:
		void update();
		void onResolverResults(net::AddressResolver* ar);
		void ownNodeLookupFinished(Task* t);
		void expireDatabaseItems();
		
	private:
		Node* node;
		RPCServer* srv;
		Database* db;
		TaskManager* tman;
		QTimer expire_timer;
		QString table_file;
		QTimer update_timer;
		NodeLookup* our_node_lookup;
	};

}

#endif
