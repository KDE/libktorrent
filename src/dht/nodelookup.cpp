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
#include "nodelookup.h"
#include <util/log.h>
#include <torrent/globals.h>
#include "rpcmsg.h"
#include "node.h"
#include "pack.h"
#include "kbucket.h"
#include "findnodereq.h"
#include "findnodersp.h"

using namespace bt;

namespace dht
{

	NodeLookup::NodeLookup(const dht::Key & key, RPCServer* rpc, Node* node, QObject* parent)
			: Task(rpc, node, parent),
			node_id(key),
			num_nodes_rsp(0)
	{
	}

	NodeLookup::~NodeLookup()
	{
	}

	void NodeLookup::handleNodes(const QByteArray& nodes, int ip_version)
	{
		Uint32 address_size = ip_version == 4 ? 26 : 38;
		Uint32 nnodes = nodes.size() / address_size;
		for (Uint32 j = 0;j < nnodes;j++)
		{
			// Unpack an entry and add it to the todo list
			try
			{
				KBucketEntry e = UnpackBucketEntry(nodes, j * address_size, ip_version);
				// Let's not talk to ourself
				if (e.getID() != node->getOurID() && !todo.contains(e) && !visited.contains(e))
					todo.insert(e);
			}
			catch (...)
			{
				// Bad data, just ignore it
			}
		}
	}

	void NodeLookup::callFinished(RPCCall* , RPCMsg::Ptr rsp)
	{
		//Out(SYS_DHT|LOG_DEBUG) << "NodeLookup::callFinished" << endl;
		if (isFinished())
			return;

		// Check the response and see if it is a good one
		if (rsp->getMethod() == dht::FIND_NODE && rsp->getType() == dht::RSP_MSG)
		{
			FindNodeRsp::Ptr fnr = rsp.dynamicCast<FindNodeRsp>();
			if (!fnr)
				return;

			const QByteArray & nodes = fnr->getNodes();
			if (nodes.size() > 0)
				handleNodes(nodes, 4);

			const QByteArray & nodes6 = fnr->getNodes6();
			if (nodes6.size() > 0)
				handleNodes(nodes6, 6);
			num_nodes_rsp++;
		}
	}

	void NodeLookup::callTimeout(RPCCall*)
	{
		//Out(SYS_DHT|LOG_DEBUG) << "NodeLookup::callTimeout" << endl;
	}

	void NodeLookup::update()
	{
		//Out(SYS_DHT|LOG_DEBUG) << "NodeLookup::update" << endl;
		//Out(SYS_DHT|LOG_DEBUG) << "todo = " << todo.count() << " ; visited = " << visited.count() << endl;

		// Go over the todo list and send find node calls until we have
		// nothing left
		while (!todo.empty() && canDoRequest())
		{
			KBucketEntrySet::iterator itr = todo.begin();
			// Only send a findNode if we haven't allrready visited the node
			if (!visited.contains(*itr))
			{
				// Send a findNode to the node
				RPCMsg::Ptr fnr(new FindNodeReq(node->getOurID(), node_id));
				fnr->setOrigin(itr->getAddress());
				rpcCall(fnr);
				visited.insert(*itr);
			}
			// Remove the entry from the todo list
			todo.erase(itr);
		}

		if (todo.empty() && getNumOutstandingRequests() == 0 && !isFinished())
		{
			Out(SYS_DHT | LOG_NOTICE) << "DHT: NodeLookup done" << endl;
			done();
		}
		else if (visited.size() > 200)
		{
			// Don't let the task run forever
			Out(SYS_DHT | LOG_NOTICE) << "DHT: NodeLookup done" << endl;
			done();
		}
	}

}
