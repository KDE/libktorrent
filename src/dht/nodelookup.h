/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTNODELOOKUP_H
#define DHTNODELOOKUP_H

#include "key.h"
#include "task.h"

namespace dht
{
class Node;
class RPCServer;

/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * \brief Task to do a node lookup.
 */
class NodeLookup : public Task
{
public:
    NodeLookup(const dht::Key &node_id, RPCServer *rpc, Node *node, QObject *parent);
    ~NodeLookup() override;

    void update() override;
    void callFinished(RPCCall *c, RPCMsg *rsp) override;
    void callTimeout(RPCCall *c) override;

private:
    void handleNodes(const QByteArray &nodes, int ip_version);

private:
    dht::Key node_id;
    bt::Uint32 num_nodes_rsp;
};

}

#endif
