/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "peermanager.h"

#include <KLocalizedString>
#include <QDateTime>
#include <QFile>
#include <QList>
#include <QSet>
#include <QTextStream>
#include <QtAlgorithms>

#include "authenticate.h"
#include "authenticationmonitor.h"
#include "chunkcounter.h"
#include "connectionlimit.h"
#include "peer.h"
#include "peerconnector.h"
#include <dht/dhtbase.h>
#include <mse/encryptedauthenticate.h>
#include <mse/encryptedpacketsocket.h>
#include <net/address.h>
#include <peer/accessmanager.h>
#include <peer/peer.h>
#include <peer/peerid.h>
#include <torrent/globals.h>
#include <torrent/server.h>
#include <torrent/torrent.h>
#include <util/bitset.h>
#include <util/error.h>
#include <util/file.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/ptrmap.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
typedef std::map<net::Address, bool>::iterator PPItr;
typedef QMap<Uint32, Peer::Ptr> PeerMap;

static ConnectionLimit climit;

class PeerManager::Private
{
public:
    Private(PeerManager *p, Torrent &tor);
    ~Private();

    void updateAvailableChunks();
    bool killBadPeer();
    void createPeer(mse::EncryptedPacketSocket::Ptr sock, const PeerID &peer_id, Uint32 support, bool local, ConnectionLimit::Token::Ptr token);
    bool connectedTo(const net::Address &addr) const;
    void update();
    void have(Peer *peer, Uint32 index);
    void connectToPeers();

public:
    PeerManager *p;
    QMap<Uint32, Peer::Ptr> peer_map;
    Torrent &tor;
    bool started;
    BitSet available_chunks, wanted_chunks;
    ChunkCounter cnt;
    bool pex_on;
    bool wanted_changed;
    PieceHandler *piece_handler;
    bool paused;
    QSet<PeerConnector::Ptr> connectors;
    QScopedPointer<SuperSeeder> superseeder;
    std::map<net::Address, bool> potential_peers;
    bool partial_seed;
    Uint32 num_cleared;
};

PeerManager::PeerManager(Torrent &tor)
    : d(new Private(this, tor))
{
}

PeerManager::~PeerManager()
{
    delete d;
}

ConnectionLimit &PeerManager::connectionLimits()
{
    return climit;
}

void PeerManager::pause()
{
    if (d->paused)
        return;

    for (Peer::Ptr p : std::as_const(d->peer_map)) {
        p->pause();
    }
    d->paused = true;
}

void PeerManager::unpause()
{
    if (!d->paused)
        return;

    for (Peer::Ptr p : std::as_const(d->peer_map)) {
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

#if 0

void PeerManager::setMaxTotalConnections(Uint32 max)
{
#ifndef Q_OS_WIN
    Uint32 sys_max = bt::MaxOpenFiles() - 50; // leave about 50 free for regular files
#else
    Uint32 sys_max = 9999; // there isn't a real limit on windows
#endif
    max_total_connections = max;
    if (max == 0 || max_total_connections > sys_max)
        max_total_connections = sys_max;
}
#endif

void PeerManager::setWantedChunks(const BitSet &bs)
{
    d->wanted_chunks = bs;
    d->wanted_changed = true;
}

void PeerManager::addPotentialPeer(const net::Address &addr, bool local)
{
    if (d->potential_peers.size() < 500) {
        d->potential_peers[addr] = local;
    }
}

void PeerManager::killSeeders()
{
    for (Peer::Ptr peer : std::as_const(d->peer_map)) {
        if (peer->isSeeder())
            peer->kill();
    }
}

void PeerManager::killUninterested()
{
    QTime now = QTime::currentTime();
    for (Peer::Ptr peer : std::as_const(d->peer_map)) {
        if (!peer->isInterested() && (peer->getConnectTime().secsTo(now) > 30))
            peer->kill();
    }
}

void PeerManager::have(Peer *p, Uint32 index)
{
    d->have(p, index);
}

void PeerManager::bitSetReceived(Peer *p, const BitSet &bs)
{
    bool interested = false;
    for (Uint32 i = 0; i < bs.getNumBits(); i++) {
        if (bs.get(i)) {
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

void PeerManager::newConnection(mse::EncryptedPacketSocket::Ptr sock, const PeerID &peer_id, Uint32 support)
{
    if (!d->started)
        return;

    ConnectionLimit::Token::Ptr token = climit.acquire(d->tor.getInfoHash());
    if (!token) {
        d->killBadPeer();
        token = climit.acquire(d->tor.getInfoHash());
    }

    if (token)
        d->createPeer(sock, peer_id, support, false, token);
}

void PeerManager::peerAuthenticated(bt::Authenticate *auth, bt::PeerConnector::WPtr pcon, bool ok, bt::ConnectionLimit::Token::Ptr token)
{
    if (d->started) {
        if (ok && !connectedTo(auth->getPeerID()))
            d->createPeer(auth->getSocket(), auth->getPeerID(), auth->supportedExtensions(), auth->isLocal(), token);
    }

    PeerConnector::Ptr ptr = pcon.toStrongRef();
    d->connectors.remove(ptr);
}

bool PeerManager::connectedTo(const PeerID &peer_id)
{
    if (!d->started)
        return false;

    for (const Peer::Ptr &p : std::as_const(d->peer_map)) {
        if (p->getPeerID() == peer_id)
            return true;
    }
    return false;
}

void PeerManager::closeAllConnections()
{
    d->peer_map.clear();
}

void PeerManager::savePeerList(const QString &file)
{
    // Lets save the entries line based
    QFile fptr(file);
    if (!fptr.open(QIODevice::WriteOnly))
        return;

    try {
        Out(SYS_GEN | LOG_DEBUG) << "Saving list of peers to " << file << endl;

        QTextStream out(&fptr);
        // first the active peers
        for (const Peer::Ptr &p : std::as_const(d->peer_map)) {
            const net::Address &addr = p->getAddress();
            out << addr.toString() << " " << (unsigned short)addr.port() << Qt::endl;
        }

        // now the potential_peers
        std::map<net::Address, bool>::const_iterator i = d->potential_peers.cbegin();
        while (i != d->potential_peers.cend()) {
            out << i->first.toString() << " " << i->first.port() << Qt::endl;
            ++i;
        }
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_DEBUG) << "Error happened during saving of peer list : " << err.toString() << endl;
    }
}

void PeerManager::loadPeerList(const QString &file)
{
    QFile fptr(file);
    if (!fptr.open(QIODevice::ReadOnly))
        return;

    try {
        Out(SYS_GEN | LOG_DEBUG) << "Loading list of peers from " << file << endl;

        while (!fptr.atEnd()) {
            QStringList sl = QString(fptr.readLine()).split(' '_L1);
            if (sl.count() != 2)
                continue;

            bool ok = false;
            net::Address addr(sl[0], sl[1].toInt(&ok));
            if (ok)
                addPotentialPeer(addr, false);
        }
    } catch (bt::Error &err) {
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

Peer::Ptr PeerManager::findPeer(PieceDownloader *pd)
{
    for (Peer::Ptr p : std::as_const(d->peer_map)) {
        if ((PieceDownloader *)p->getPeerDownloader() == pd)
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

void PeerManager::peerSourceReady(PeerSource *ps)
{
    net::Address addr;
    bool local = false;
    while (ps->takePeer(addr, local))
        addPotentialPeer(addr, local);
}

void PeerManager::pex(const QByteArray &arr, int ip_version)
{
    if (!d->pex_on)
        return;

    if (ip_version == 4) {
        Out(SYS_CON | LOG_NOTICE) << "PEX: found " << (arr.size() / 6) << " IPv4 peers" << endl;
        for (int i = 0; i + 6 <= arr.size(); i += 6) {
            const Uint8 *tmp = (const Uint8 *)arr.data() + i;
            addPotentialPeer(net::Address(ReadUint32(tmp, 0), ReadUint16(tmp, 4)), false);
        }
    } else if (ip_version == 6) {
        Out(SYS_CON | LOG_NOTICE) << "PEX: found " << (arr.size() / 18) << " IPv6 peers" << endl;
        for (int i = 0; i < arr.size(); i += 18) {
            Q_IPV6ADDR ip;
            memcpy(ip.c, arr.data() + i, 16);
            quint16 port = ReadUint16((const Uint8 *)arr.data() + i, 16);
            addPotentialPeer(net::Address(ip, port), false);
        }
    }
}

void PeerManager::setPexEnabled(bool on)
{
    if (on && d->tor.isPrivate())
        return;

    if (d->pex_on == on)
        return;

    for (Peer::Ptr p : std::as_const(d->peer_map)) {
        if (!p->isKilled()) {
            p->setPexEnabled(on);
            bt::Uint16 port = ServerInterface::getPort();
            p->sendExtProtHandshake(port, d->tor.getMetaData().size(), d->partial_seed);
        }
    }
    d->pex_on = on;
}

void PeerManager::setGroupIDs(Uint32 up, Uint32 down)
{
    for (Peer::Ptr p : std::as_const(d->peer_map))
        p->setGroupIDs(up, down);
}

void PeerManager::portPacketReceived(const QString &ip, Uint16 port)
{
    if (Globals::instance().getDHT().isRunning() && !d->tor.isPrivate())
        Globals::instance().getDHT().portReceived(ip, port);
}

void PeerManager::pieceReceived(const Piece &p)
{
    if (d->piece_handler)
        d->piece_handler->pieceReceived(p);
}

void PeerManager::setPieceHandler(PieceHandler *ph)
{
    d->piece_handler = ph;
}

void PeerManager::killStalePeers()
{
    for (Peer::Ptr p : std::as_const(d->peer_map)) {
        if (p->getDownloadRate() == 0 && p->getUploadRate() == 0)
            p->kill();
    }
}

void PeerManager::setSuperSeeding(bool on, const BitSet &chunks)
{
    Q_UNUSED(chunks);
    if ((d->superseeder && on) || (!d->superseeder && !on))
        return;

    d->superseeder.reset(on ? new SuperSeeder(d->cnt.getNumChunks()) : nullptr);

    // When entering or exiting superseeding mode kill all peers
    // but first add the current list to the potential_peers list, so we can reconnect later.
    for (Peer::Ptr p : std::as_const(d->peer_map)) {
        const net::Address &addr = p->getAddress();
        addPotentialPeer(addr, false);
        p->kill();
    }
}

void PeerManager::sendHave(Uint32 index)
{
    if (d->superseeder)
        return;

    for (Peer::Ptr peer : std::as_const(d->peer_map)) {
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
    for (const Peer::Ptr &peer : std::as_const(d->peer_map)) {
        if (!peer->isSeeder())
            cnt++;
    }

    return cnt;
}

Uint32 PeerManager::getNumConnectedSeeders() const
{
    Uint32 cnt = 0;
    for (const Peer::Ptr &peer : std::as_const(d->peer_map)) {
        if (peer->isSeeder())
            cnt++;
    }

    return cnt;
}

Uint32 PeerManager::getNumPending() const
{
    return d->connectors.size();
}

const bt::BitSet &PeerManager::getAvailableChunksBitSet() const
{
    return d->available_chunks;
}

ChunkCounter &PeerManager::getChunkCounter()
{
    return d->cnt;
}

bool PeerManager::isPexEnabled() const
{
    return d->pex_on;
}

const bt::Torrent &PeerManager::getTorrent() const
{
    return d->tor;
}

bool PeerManager::isStarted() const
{
    return d->started;
}

void PeerManager::visit(PeerManager::PeerVisitor &visitor)
{
    for (const Peer::Ptr &p : std::as_const(d->peer_map)) {
        visitor.visit(p);
    }
}

Uint32 PeerManager::uploadRate() const
{
    Uint32 rate = 0;
    for (const Peer::Ptr &p : std::as_const(d->peer_map)) {
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
    if (d->partial_seed != partial_seed) {
        d->partial_seed = partial_seed;

        // If partial seeding status changes, update all peers
        bt::Uint16 port = ServerInterface::getPort();
        for (Peer::Ptr peer : std::as_const(d->peer_map)) {
            peer->sendExtProtHandshake(port, d->tor.getMetaData().size(), d->partial_seed);
        }
    }
}

bool PeerManager::isPartialSeed() const
{
    return d->partial_seed;
}

//////////////////////////////////////////////////

PeerManager::Private::Private(PeerManager *p, Torrent &tor)
    : p(p)
    , tor(tor)
    , available_chunks(tor.getNumChunks())
    , wanted_chunks(tor.getNumChunks())
    , cnt(tor.getNumChunks())
    , partial_seed(false)
    , num_cleared(0)
{
    started = false;
    wanted_chunks.setAll(true);
    wanted_changed = false;
    pex_on = !tor.isPrivate();
    piece_handler = nullptr;
    paused = false;
}

PeerManager::Private::~Private()
{
    ServerInterface::removePeerManager(p);
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
    for (PeerMap::iterator i = peer_map.begin(); i != peer_map.end();) {
        Peer::Ptr peer = i.value();
        if (!peer->isKilled() && peer->isStalled()) {
            p->addPotentialPeer(peer->getAddress(), peer->getStats().local);
            peer->kill();
        }

        if (peer->isKilled()) {
            cnt.decBitSet(peer->getBitSet());
            updateAvailableChunks();
            i = peer_map.erase(i);
            Q_EMIT p->peerKilled(peer.data());
            if (superseeder)
                superseeder->peerRemoved(peer.data());
            num_cleared++;
        } else {
            peer->update();
            if (wanted_changed) {
                if (peer->hasWantedChunks(wanted_chunks))
                    peer->sendInterested();
                else
                    peer->sendNotInterested();
            }
            ++i;
        }
    }

    wanted_changed = false;
    connectToPeers();
}

void PeerManager::Private::have(Peer *peer, Uint32 index)
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
    for (Uint32 i = 0; i < available_chunks.getNumBits(); i++) {
        available_chunks.set(i, cnt.get(i) > 0);
    }
}

bool PeerManager::Private::killBadPeer()
{
    for (Peer::Ptr peer : std::as_const(peer_map)) {
        if (peer->getStats().aca_score <= -5.0 && peer->getStats().aca_score > -50.0) {
            Out(SYS_GEN | LOG_DEBUG) << "Killing bad peer, to make room for other peers" << endl;
            peer->kill();
            return true;
        }
    }
    return false;
}

void PeerManager::Private::createPeer(mse::EncryptedPacketSocket::Ptr sock,
                                      const bt::PeerID &peer_id,
                                      Uint32 support,
                                      bool local,
                                      bt::ConnectionLimit::Token::Ptr token)
{
    Peer::Ptr peer(new Peer(sock, peer_id, tor.getNumChunks(), tor.getChunkSize(), support, local, token, p));
    peer_map.insert(peer->getID(), peer);
    Q_EMIT p->newPeer(peer.data());
    peer->setPexEnabled(pex_on);
    // send extension protocol handshake
    bt::Uint16 port = ServerInterface::getPort();
    peer->sendExtProtHandshake(port, tor.getMetaData().size(), partial_seed);

    if (superseeder)
        superseeder->peerAdded(peer.data());
}

bool PeerManager::Private::connectedTo(const net::Address &addr) const
{
    PeerMap::const_iterator i = peer_map.begin();
    while (i != peer_map.end()) {
        if (i.value()->getAddress() == addr)
            return true;
        ++i;
    }
    return false;
}

void PeerManager::Private::connectToPeers()
{
    if (paused || potential_peers.size() == 0 || (Uint32)connectors.size() > MAX_SIMULTANIOUS_AUTHS)
        return;

    while (!potential_peers.empty()) {
        if ((Uint32)connectors.size() > MAX_SIMULTANIOUS_AUTHS)
            return;

        PPItr itr = potential_peers.begin();

        AccessManager &aman = AccessManager::instance();

        if (aman.allowed(itr->first) && !connectedTo(itr->first)) {
            ConnectionLimit::Token::Ptr token = climit.acquire(tor.getInfoHash());
            if (!token)
                break;

            PeerConnector::Ptr pcon(new PeerConnector(itr->first, itr->second, p, token));
            pcon->setWeakPointer(PeerConnector::WPtr(pcon));
            connectors.insert(pcon);
            pcon->start();
        }

        potential_peers.erase(itr);
    }
}

}

#include "moc_peermanager.cpp"
