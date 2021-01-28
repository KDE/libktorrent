/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "serverinterface.h"
#include <QHostAddress>
#include <mse/encryptedpacketsocket.h>
#include <mse/encryptedserverauthenticate.h>
#include <peer/accessmanager.h>
#include <peer/authenticationmonitor.h>
#include <peer/peermanager.h>
#include <peer/serverauthenticate.h>
#include <torrent/torrent.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/sha1hash.h>

namespace bt
{
QList<PeerManager *> ServerInterface::peer_managers;
bool ServerInterface::encryption = false;
bool ServerInterface::allow_unencrypted = true;
Uint16 ServerInterface::port = 6881;
bool ServerInterface::utp_enabled = false;
bool ServerInterface::only_use_utp = false;
TransportProtocol ServerInterface::primary_transport_protocol = TCP;

ServerInterface::ServerInterface(QObject *parent)
    : QObject(parent)
{
}

ServerInterface::~ServerInterface()
{
}

void ServerInterface::addPeerManager(PeerManager *pman)
{
    peer_managers.append(pman);
}

void ServerInterface::removePeerManager(PeerManager *pman)
{
    peer_managers.removeAll(pman);
}

PeerManager *ServerInterface::findPeerManager(const bt::SHA1Hash &hash)
{
    QList<PeerManager *>::iterator i = peer_managers.begin();
    while (i != peer_managers.end()) {
        PeerManager *pm = *i;
        if (pm && pm->getTorrent().getInfoHash() == hash) {
            if (!pm->isStarted())
                return 0;
            else
                return pm;
        }
        ++i;
    }
    return 0;
}

bool ServerInterface::findInfoHash(const bt::SHA1Hash &skey, SHA1Hash &info_hash)
{
    Uint8 buf[24];
    memcpy(buf, "req2", 4);
    QList<PeerManager *>::iterator i = peer_managers.begin();
    while (i != peer_managers.end()) {
        PeerManager *pm = *i;
        memcpy(buf + 4, pm->getTorrent().getInfoHash().getData(), 20);
        if (SHA1Hash::generate(buf, 24) == skey) {
            info_hash = pm->getTorrent().getInfoHash();
            return true;
        }
        ++i;
    }
    return false;
}

void ServerInterface::disableEncryption()
{
    encryption = false;
}

void ServerInterface::enableEncryption(bool unencrypted_allowed)
{
    encryption = true;
    allow_unencrypted = unencrypted_allowed;
}

QStringList ServerInterface::bindAddresses()
{
    QString iface = NetworkInterface();
    QStringList ips = NetworkInterfaceIPAddresses(iface);
    if (ips.count() == 0) {
        // Interface does not exist, so add any addresses
        ips << QHostAddress(QHostAddress::AnyIPv6).toString() << QHostAddress(QHostAddress::Any).toString();
    }

    return ips;
}

void ServerInterface::newConnection(mse::EncryptedPacketSocket::Ptr s)
{
    if (peer_managers.count() == 0) {
        s->close();
    } else {
        if (!AccessManager::instance().allowed(s->getRemoteAddress())) {
            Out(SYS_CON | LOG_DEBUG) << "A client with a blocked IP address (" << s->getRemoteIPAddress() << ") tried to connect !" << endl;
            return;
        }

        // Not enough free file descriptors
        if (!OpenFileAllowed())
            return;

        ServerAuthenticate *auth = 0;

        if (encryption)
            auth = new mse::EncryptedServerAuthenticate(s);
        else
            auth = new ServerAuthenticate(s);

        AuthenticationMonitor::instance().add(auth);
    }
}

void ServerInterface::setPrimaryTransportProtocol(TransportProtocol proto)
{
    primary_transport_protocol = proto;
}

void ServerInterface::setUtpEnabled(bool on, bool only_utp)
{
    utp_enabled = on;
    only_use_utp = only_utp;
}

}
