/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTDHT_H
#define DHTDHT_H

#include "dhtbase.h"
#include "key.h"
#include "rpcmsg.h"
#include <QMap>
#include <QString>
#include <QTimer>
#include <util/constants.h>
#include <util/timer.h>

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

/*!
    \author Joris Guisson <joris.guisson@gmail.com>
    \brief Handles everything to do with DHT.
*/
class DHT : public DHTBase
{
    Q_OBJECT
public:
    DHT();
    ~DHT() override;

    void ping(const PingReq &r);
    void findNode(const FindNodeReq &r);
    void response(const RPCMsg &r);
    void getPeers(const GetPeersReq &r);
    void announce(const AnnounceReq &r);
    void error(const ErrMsg &r);
    void timeout(const RPCMsg &r);

    /*!
     * A Peer has received a PORT message, and uses this function to alert the DHT of it.
     * \param ip The IP of the peer
     * \param port The port in the PORT message
     */
    void portReceived(const QString &ip, bt::Uint16 port) override;

    /*!
     * Do an announce on the DHT network
     * \param info_hash The info_hash
     * \param port The port
     * \return The task which handles this
     */
    AnnounceTask *announce(const bt::SHA1Hash &info_hash, bt::Uint16 port) override;

    /*!
     * Refresh a bucket using a find node task.
     * \param id The id
     * \param bucket The bucket to refresh
     */
    NodeLookup *refreshBucket(const dht::Key &id, KBucket &bucket);

    /*!
     * Do a NodeLookup.
     * \param id The id of the key to search
     */
    NodeLookup *findNode(const dht::Key &id);

    //! Do a findNode for our node id
    NodeLookup *findOwnNode();

    //! See if it is possible to start a task
    bool canStartTask() const;

    void start(const QString &table, const QString &key_file, bt::Uint16 port) override;
    void stop() override;
    void addDHTNode(const QString &host, bt::Uint16 hport) override;

    QMap<QString, int> getClosestGoodNodes(int maxNodes) override;

    //! Bootstrap from well-known nodes
    void bootstrap();

private Q_SLOTS:
    void update() override;
    void onResolverResults(net::AddressResolver *ar);
    void ownNodeLookupFinished(dht::Task *t);
    void expireDatabaseItems();

private:
    Node *node;
    RPCServer *srv;
    Database *db;
    TaskManager *tman;
    QTimer expire_timer;
    QString table_file;
    QTimer update_timer;
    NodeLookup *our_node_lookup;
};

}

#endif
