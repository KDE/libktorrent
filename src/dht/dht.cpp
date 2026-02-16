/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "dht.h"
#include "announcereq.h"
#include "announcersp.h"
#include "announcetask.h"
#include "database.h"
#include "findnodereq.h"
#include "findnodersp.h"
#include "getpeersreq.h"
#include "getpeersrsp.h"
#include "kclosestnodessearch.h"
#include "node.h"
#include "nodelookup.h"
#include "pingreq.h"
#include "pingrsp.h"
#include "rpcmsg.h"
#include "rpcserver.h"
#include "taskmanager.h"
#include <QMap>
#include <bcodec/bnode.h>
#include <net/addressresolver.h>
#include <util/array.h>
#include <util/error.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
DHT::DHT()
    : node(nullptr)
    , srv(nullptr)
    , db(nullptr)
    , tman(nullptr)
    , our_node_lookup(nullptr)
{
    connect(&update_timer, &QTimer::timeout, this, &DHT::update);
    connect(&expire_timer, &QTimer::timeout, this, &DHT::expireDatabaseItems);
}

DHT::~DHT()
{
    if (running) {
        stop();
    }
}

void DHT::start(const QString &table, const QString &key_file, bt::Uint16 port)
{
    if (running) {
        return;
    }

    if (port == 0) {
        port = 6881;
    }

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
    expire_timer.start(5 * 60 * 1000);
    Q_EMIT started();
    if (node->getNumEntriesInRoutingTable() > 0) {
        // refresh the DHT table by looking for our own ID
        findOwnNode();
    } else {
        // routing table is empty, bootstrap from well-known nodes
        Out(SYS_DHT | LOG_NOTICE) << "DHT: Routing table empty, bootstrapping from well-known nodes" << endl;
        bootstrap();
    }
}

void DHT::stop()
{
    if (!running) {
        return;
    }

    update_timer.stop();
    expire_timer.stop();
    Out(SYS_DHT | LOG_NOTICE) << "DHT: Stopping " << endl;
    srv->stop();
    node->saveTable(table_file);
    running = false;
    Q_EMIT stopped();
    delete tman;
    tman = nullptr;
    delete db;
    db = nullptr;
    delete node;
    node = nullptr;
    delete srv;
    srv = nullptr;
}

void DHT::ping(const PingReq &r)
{
    if (!running) {
        return;
    }

    // ignore requests we get from ourself
    if (r.getID() == node->getOurID()) {
        return;
    }

    PingRsp rsp(r.getMTID(), node->getOurID());
    rsp.setOrigin(r.getOrigin());
    srv->sendMsg(rsp);
    node->received(this, r);
}

void DHT::findNode(const dht::FindNodeReq &r)
{
    if (!running) {
        return;
    }

    // ignore requests we get from ourself
    if (r.getID() == node->getOurID()) {
        return;
    }

    node->received(this, r);
    // find the K closest nodes and pack them
    KClosestNodesSearch kns(r.getTarget(), K);

    bt::Uint32 wants = 0;
    if (r.wants(4) || r.getOrigin().ipVersion() == 4) {
        wants |= WANT_IPV4;
    }
    if (r.wants(6) || r.getOrigin().ipVersion() == 6) {
        wants |= WANT_IPV6;
    }

    node->findKClosestNodes(kns, wants);

    FindNodeRsp fnr(r.getMTID(), node->getOurID());
    // pack the found nodes in a byte array
    kns.pack(&fnr);
    fnr.setOrigin(r.getOrigin());
    srv->sendMsg(fnr);
}

NodeLookup *DHT::findOwnNode()
{
    if (our_node_lookup) {
        return our_node_lookup;
    }

    our_node_lookup = findNode(node->getOurID());
    if (our_node_lookup) {
        connect(our_node_lookup, &NodeLookup::finished, this, &DHT::ownNodeLookupFinished);
    }
    return our_node_lookup;
}

void DHT::ownNodeLookupFinished(Task *t)
{
    if (our_node_lookup == t) {
        our_node_lookup = nullptr;
    }
}

void DHT::announce(const AnnounceReq &r)
{
    if (!running) {
        return;
    }

    // ignore requests we get from ourself
    if (r.getID() == node->getOurID()) {
        return;
    }

    node->received(this, r);
    // first check if the token is OK
    QByteArray token = r.getToken();
    if (!db->checkToken(token, r.getOrigin())) {
        return;
    }

    // everything OK, so store the value
    db->store(r.getInfoHash(), DBItem(r.getOrigin()));
    // send a proper response to indicate everything is OK
    AnnounceRsp rsp(r.getMTID(), node->getOurID());
    rsp.setOrigin(r.getOrigin());
    srv->sendMsg(rsp);
}

void DHT::getPeers(const GetPeersReq &r)
{
    if (!running) {
        return;
    }

    // ignore requests we get from ourself
    if (r.getID() == node->getOurID()) {
        return;
    }

    node->received(this, r);
    DBItemList dbl;
    db->sample(r.getInfoHash(), dbl, 50, r.getOrigin().ipVersion());

    // generate a token
    QByteArray token = db->genToken(r.getOrigin());

    bt::Uint32 wants = 0;
    if (r.wants(4) || r.getOrigin().ipVersion() == 4) {
        wants |= WANT_IPV4;
    }
    if (r.wants(6) || r.getOrigin().ipVersion() == 6) {
        wants |= WANT_IPV6;
    }

    // if data is null do the same as when we have a findNode request
    // find the K closest nodes and pack them
    KClosestNodesSearch kns(r.getInfoHash(), K);
    node->findKClosestNodes(kns, wants);

    GetPeersRsp fnr(r.getMTID(), node->getOurID(), dbl, token);
    kns.pack(&fnr);
    fnr.setOrigin(r.getOrigin());
    srv->sendMsg(fnr);
}

void DHT::response(const RPCMsg &r)
{
    if (!running) {
        return;
    }

    node->received(this, r);
}

void DHT::error(const ErrMsg &msg)
{
    Q_UNUSED(msg);
}

void DHT::portReceived(const QString &ip, bt::Uint16 port)
{
    if (!running) {
        return;
    }

    auto r = std::make_unique<PingReq>(node->getOurID());
    r->setOrigin(net::Address(ip, port));
    srv->doCall(std::move(r));
}

bool DHT::canStartTask() const
{
    // we can start a task if we have less then  7 runnning and
    // there are at least 16 RPC slots available
    if (tman->getNumTasks() >= 7) {
        return false;
    } else if (256 - srv->getNumActiveRPCCalls() <= 16) {
        return false;
    }

    return true;
}

AnnounceTask *DHT::announce(const bt::SHA1Hash &info_hash, bt::Uint16 port)
{
    if (!running) {
        return nullptr;
    }

    KClosestNodesSearch kns(info_hash, K);
    node->findKClosestNodes(kns, WANT_BOTH);
    if (kns.getNumEntries() > 0) {
        Out(SYS_DHT | LOG_NOTICE) << "DHT: Doing announce " << endl;
        AnnounceTask *at = new AnnounceTask(db, srv, node, info_hash, port, tman);
        at->start(kns, !canStartTask());
        tman->addTask(at);
        if (!db->contains(info_hash)) {
            db->insert(info_hash);
        }
        return at;
    }

    return nullptr;
}

NodeLookup *DHT::refreshBucket(const dht::Key &id, KBucket &bucket)
{
    if (!running) {
        return nullptr;
    }

    KClosestNodesSearch kns(id, K);
    bucket.findKClosestNodes(kns);
    bucket.updateRefreshTimer();
    if (kns.getNumEntries() > 0) {
        Out(SYS_DHT | LOG_DEBUG) << "DHT: refreshing bucket " << endl;
        NodeLookup *nl = new NodeLookup(id, srv, node, tman);
        nl->start(kns, !canStartTask());
        tman->addTask(nl);
        return nl;
    }

    return nullptr;
}

NodeLookup *DHT::findNode(const dht::Key &id)
{
    if (!running) {
        return nullptr;
    }

    KClosestNodesSearch kns(id, K);
    node->findKClosestNodes(kns, WANT_BOTH);
    if (kns.getNumEntries() > 0) {
        Out(SYS_DHT | LOG_DEBUG) << "DHT: finding node " << endl;
        NodeLookup *at = new NodeLookup(id, srv, node, tman);
        at->start(kns, !canStartTask());
        tman->addTask(at);
        return at;
    }

    return nullptr;
}

void DHT::expireDatabaseItems()
{
    db->expire(bt::CurrentTime());
}

void DHT::update()
{
    if (!running) {
        return;
    }

    try {
        node->refreshBuckets(this);
        stats.num_tasks = tman->getNumTasks() + tman->getNumQueuedTasks();
        stats.num_peers = node->getNumEntriesInRoutingTable();
    } catch (bt::Error &e) {
        Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Error: " << e.toString() << endl;
    }
}

void DHT::timeout(const RPCMsg &r)
{
    node->onTimeout(r);
}

void DHT::addDHTNode(const QString &host, Uint16 hport)
{
    if (!running) {
        return;
    }

    net::Address addr;
    if (addr.setAddress(host)) {
        Out(SYS_DHT | LOG_DEBUG) << "DHT: Adding node '" << host << ":" << hport << "'" << endl;
        addr.setPort(hport);
        srv->ping(node->getOurID(), addr);
    } else {
        Out(SYS_DHT | LOG_DEBUG) << "DHT: Resolving node '" << host << "'" << endl;
        net::AddressResolver::resolve(host, hport, this, SLOT(onResolverResults(net::AddressResolver *)));
    }
}

void DHT::onResolverResults(net::AddressResolver *res)
{
    if (!running) {
        return;
    }

    if (res->succeeded()) {
        Out(SYS_DHT | LOG_DEBUG) << "DHT: Adding node '" << res->address().toString() << ":" << res->address().port() << "'" << endl;
        srv->ping(node->getOurID(), res->address());
    }
}

QMap<QString, int> DHT::getClosestGoodNodes(int maxNodes)
{
    QMap<QString, int> map;

    if (!node) {
        return map;
    }

    int max = 0;
    KClosestNodesSearch kns(node->getOurID(), maxNodes * 2);
    node->findKClosestNodes(kns, WANT_BOTH);

    KClosestNodesSearch::Itr it;
    for (it = kns.begin(); it != kns.end(); ++it) {
        KBucketEntry e = it->second;

        if (!e.isGood()) {
            continue;
        }

        const net::Address &a = e.getAddress();

        map.insert(a.toString(), a.port());
        if (++max >= maxNodes) {
            break;
        }
    }

    return map;
}

void DHT::bootstrap()
{
    Out(SYS_DHT | LOG_DEBUG) << "DHT: Adding well-known bootstrap nodes" << endl;
    addDHTNode(u"router.bittorrent.com"_s, 6881);
    addDHTNode(u"router.utorrent.com"_s, 6881);
    addDHTNode(u"dht.libtorrent.org"_s, 25401);
    addDHTNode(u"dht.transmissionbt.com"_s, 6881);
}

}

#include "moc_dht.cpp"
