/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "peerconnector.h"
#include "authenticationmonitor.h"
#include "peermanager.h"
#include <QPointer>
#include <QSet>
#include <interfaces/serverinterface.h>
#include <mse/encryptedauthenticate.h>
#include <torrent/torrent.h>
#include <util/functions.h>

namespace bt
{
static ResourceManager half_open_connections(50);

class PeerConnector::Private
{
public:
    Private(PeerConnector *p, const net::Address &addr, bool local, Uint32 protocol_version, PeerManager *pman, ConnectionLimit::Token::Ptr token)
        : p(p)
        , addr(addr)
        , local(local)
        , pman(pman)
        , stopping(false)
        , do_not_start(false)
        , token(token)
        , protocol_version(protocol_version)
    {
    }

    ~Private()
    {
        if (auth.data()) {
            stopping = true;
            auth.data()->stop();
            stopping = false;
        }
    }

    void start(Method method);
    void authenticationFinished(Authenticate *auth, bool ok);

public:
    PeerConnector *p;
    QSet<Method> tried_methods;
    Method current_method;
    net::Address addr;
    bool local;
    QPointer<PeerManager> pman;
    QPointer<Authenticate> auth;
    bool stopping;
    bool do_not_start;
    PeerConnector::WPtr self;
    ConnectionLimit::Token::Ptr token;
    Uint32 protocol_version = 1;
};

PeerConnector::PeerConnector(const net::Address &addr, bool local, Uint32 protocol_version, bt::PeerManager *pman, ConnectionLimit::Token::Ptr token)
    : Resource(&half_open_connections, pman->getTorrent().getInfoHash().truncated().toString())
    , d(std::make_unique<Private>(this, addr, local, protocol_version, pman, token))
{
}

PeerConnector::~PeerConnector()
{
}

void PeerConnector::setWeakPointer(PeerConnector::WPtr ptr)
{
    d->self = ptr;
}

void PeerConnector::setMaxActive(Uint32 mc)
{
    half_open_connections.setMaxActive(mc);
}

void PeerConnector::start()
{
    half_open_connections.add(this);
}

void PeerConnector::acquired()
{
    PeerManager *pm = d->pman.data();
    if (!pm || !pm->isStarted())
        return;

    bt::TransportProtocol primary = ServerInterface::primaryTransportProtocol();
    bool encryption = ServerInterface::isEncryptionEnabled();
    bool utp = ServerInterface::isUtpEnabled();

    if (encryption) {
        if (utp && primary == bt::UTP)
            d->start(UTP_WITH_ENCRYPTION);
        else
            d->start(TCP_WITH_ENCRYPTION);
    } else {
        if (utp && primary == bt::UTP)
            d->start(UTP_WITHOUT_ENCRYPTION);
        else
            d->start(TCP_WITHOUT_ENCRYPTION);
    }
}

void PeerConnector::authenticationFinished(Authenticate *auth, bool ok)
{
    d->authenticationFinished(auth, ok);
}

void PeerConnector::Private::authenticationFinished(Authenticate *auth, bool ok)
{
    this->auth.clear();
    if (stopping)
        return;

    PeerManager *pm = pman.data();
    if (!pm)
        return;

    if (ok) {
        if (protocol_version == 1 && auth->supportsV2()) {
            ++protocol_version;
            tried_methods.clear();
            start(current_method);
            return;
        } else {
            pm->peerAuthenticated(auth, self, ok, token);
            return;
        }
    }

    tried_methods.insert(current_method);

    bt::TransportProtocol primary = ServerInterface::primaryTransportProtocol();
    //      QList<Method> allowed;

    bool tcp_allowed = OpenFileAllowed();
    bool encryption = ServerInterface::isEncryptionEnabled();
    bool only_use_encryption = !ServerInterface::unencryptedConnectionsAllowed();
    bool utp = ServerInterface::isUtpEnabled();
    bool only_use_utp = ServerInterface::onlyUseUtp();

    if (primary == bt::UTP) {
        if (utp && encryption && !tried_methods.contains(UTP_WITH_ENCRYPTION))
            start(UTP_WITH_ENCRYPTION);
        else if (utp && !only_use_encryption && !tried_methods.contains(UTP_WITHOUT_ENCRYPTION))
            start(UTP_WITHOUT_ENCRYPTION);
        else if (!only_use_utp && encryption && !tried_methods.contains(TCP_WITH_ENCRYPTION) && tcp_allowed)
            start(TCP_WITH_ENCRYPTION);
        else if (!only_use_utp && !only_use_encryption && !tried_methods.contains(TCP_WITHOUT_ENCRYPTION) && tcp_allowed)
            start(TCP_WITHOUT_ENCRYPTION);
        else if (protocol_version == 1)
            ++protocol_version;
        else
            pm->peerAuthenticated(auth, self, false, token);
    } else { // Primary is TCP
        if (!only_use_utp && encryption && !tried_methods.contains(TCP_WITH_ENCRYPTION) && tcp_allowed)
            start(TCP_WITH_ENCRYPTION);
        else if (!only_use_utp && !only_use_encryption && !tried_methods.contains(TCP_WITHOUT_ENCRYPTION) && tcp_allowed)
            start(TCP_WITHOUT_ENCRYPTION);
        else if (utp && encryption && !tried_methods.contains(UTP_WITH_ENCRYPTION))
            start(UTP_WITH_ENCRYPTION);
        else if (utp && !only_use_encryption && !tried_methods.contains(UTP_WITHOUT_ENCRYPTION))
            start(UTP_WITHOUT_ENCRYPTION);
        else if (protocol_version == 1)
            ++protocol_version;
        else
            pm->peerAuthenticated(auth, self, false, token);
    }
}

void PeerConnector::Private::start(PeerConnector::Method method)
{
    PeerManager *pm = pman.data();
    if (!pm)
        return;

    current_method = method;
    const Torrent &tor = pm->getTorrent();
    const InfoHash &info_hash = tor.getInfoHash();
    const bool we_support_v2 = info_hash.hasV2();
    if (protocol_version == 2 && !we_support_v2) {
        pm->peerAuthenticated(nullptr, self, false, token);
    }

    const SHA1Hash truncated_hash = protocol_version == 1 ? tor.getInfoHash().getV1() : tor.getInfoHash().truncated();
    TransportProtocol proto = (method == TCP_WITH_ENCRYPTION || method == TCP_WITHOUT_ENCRYPTION) ? TCP : UTP;
    if (method == TCP_WITH_ENCRYPTION || method == UTP_WITH_ENCRYPTION)
        auth = new mse::EncryptedAuthenticate(addr, proto, truncated_hash, tor.getPeerID(), self, we_support_v2);
    else
        auth = new Authenticate(addr, proto, truncated_hash, tor.getPeerID(), self, we_support_v2);

    if (local)
        auth.data()->setLocal(true);

    AuthenticationMonitor::instance().add(auth.data());
}

}
