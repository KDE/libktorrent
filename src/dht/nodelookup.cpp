/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "nodelookup.h"
#include "findnodereq.h"
#include "findnodersp.h"
#include "kbucket.h"
#include "node.h"
#include "pack.h"
#include "rpcmsg.h"
#include <torrent/globals.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
NodeLookup::NodeLookup(const dht::Key &key, RPCServer *rpc, Node *node, QObject *parent)
    : Task(rpc, node, parent)
    , node_id(key)
    , num_nodes_rsp(0)
{
}

NodeLookup::~NodeLookup()
{
}

void NodeLookup::handleNodes(const QByteArray &nodes, int ip_version)
{
    Uint32 address_size = ip_version == 4 ? 26 : 38;
    Uint32 nnodes = nodes.size() / address_size;
    for (Uint32 j = 0; j < nnodes; j++) {
        // unpack an entry and add it to the todo list
        try {
            KBucketEntry e = UnpackBucketEntry(nodes, j * address_size, ip_version);
            // lets not talk to ourself
            if (e.getID() != node->getOurID() && !todo.contains(e) && !visited.contains(e))
                todo.insert(e);
        } catch (...) {
            // bad data, just ignore it
        }
    }
}

void NodeLookup::callFinished(RPCCall *, RPCMsg::Ptr rsp)
{
    // Out(SYS_DHT|LOG_DEBUG) << "NodeLookup::callFinished" << endl;
    if (isFinished())
        return;

    // check the response and see if it is a good one
    if (rsp->getMethod() == dht::FIND_NODE && rsp->getType() == dht::RSP_MSG) {
        FindNodeRsp::Ptr fnr = rsp.dynamicCast<FindNodeRsp>();
        if (!fnr)
            return;

        const QByteArray &nodes = fnr->getNodes();
        if (nodes.size() > 0)
            handleNodes(nodes, 4);

        const QByteArray &nodes6 = fnr->getNodes6();
        if (nodes6.size() > 0)
            handleNodes(nodes6, 6);
        num_nodes_rsp++;
    }
}

void NodeLookup::callTimeout(RPCCall *)
{
    //  Out(SYS_DHT|LOG_DEBUG) << "NodeLookup::callTimeout" << endl;
}

void NodeLookup::update()
{
    //  Out(SYS_DHT|LOG_DEBUG) << "NodeLookup::update" << endl;
    //  Out(SYS_DHT|LOG_DEBUG) << "todo = " << todo.count() << " ; visited = " << visited.count() << endl;
    // go over the todo list and send find node calls
    // until we have nothing left
    while (!todo.empty() && canDoRequest()) {
        KBucketEntrySet::iterator itr = todo.begin();
        // only send a findNode if we haven't allrready visited the node
        if (!visited.contains(*itr)) {
            // send a findNode to the node
            RPCMsg::Ptr fnr(new FindNodeReq(node->getOurID(), node_id));
            fnr->setOrigin(itr->getAddress());
            rpcCall(fnr);
            visited.insert(*itr);
        }
        // remove the entry from the todo list
        todo.erase(itr);
    }

    if (todo.empty() && getNumOutstandingRequests() == 0 && !isFinished()) {
        Out(SYS_DHT | LOG_NOTICE) << "DHT: NodeLookup done" << endl;
        done();
    } else if (visited.size() > 200) {
        // don't let the task run forever
        Out(SYS_DHT | LOG_NOTICE) << "DHT: NodeLookup done" << endl;
        done();
    }
}
}
