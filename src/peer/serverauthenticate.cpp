/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "serverauthenticate.h"
#include "peerid.h"
#include "peermanager.h"
#include <mse/encryptedpacketsocket.h>
#include <peer/accessmanager.h>
#include <torrent/globals.h>
#include <torrent/server.h>
#include <torrent/torrent.h>
#include <util/log.h>
#include <util/sha1hash.h>

namespace bt
{
bool ServerAuthenticate::s_firewalled = true;

ServerAuthenticate::ServerAuthenticate(std::unique_ptr<mse::EncryptedPacketSocket> sock)
    : AuthenticateBase(std::move(sock))
{
}

ServerAuthenticate::~ServerAuthenticate()
{
}

void ServerAuthenticate::onFinish(bool succes)
{
    Out(SYS_CON | LOG_NOTICE) << "Authentication(S) to " << sock->getRemoteIPAddress() << " : " << (succes ? "ok" : "failure") << endl;
    finished = true;
    setFirewalled(false);

    if (!succes)
        sock.reset();

    timer.stop();
}

void ServerAuthenticate::handshakeReceived(bool full)
{
    Uint8 *hs = handshake;
    AccessManager &aman = AccessManager::instance();

    if (!aman.allowed(sock->getRemoteAddress())) {
        Out(SYS_GEN | LOG_NOTICE) << "The IP address " << sock->getRemoteIPAddress() << " is blocked" << endl;
        onFinish(false);
        return;
    }

    // try to find a PeerManager which has the right info hash
    SHA1Hash rh(hs + 28);
    PeerManager *pman = ServerInterface::findPeerManager(rh);
    if (!pman) {
        onFinish(false);
        return;
    }

    if (full) {
        // check if we aren't connecting to ourself
        char tmp[21];
        tmp[20] = '\0';
        memcpy(tmp, hs + 48, 20);
        PeerID peer_id = PeerID(tmp);
        if (pman->getTorrent().getPeerID() == peer_id) {
            Out(SYS_CON | LOG_NOTICE) << "Lets not connect to our self" << endl;
            onFinish(false);
            return;
        }

        // check if we aren't already connected to the client
        if (pman->connectedTo(peer_id)) {
            Out(SYS_CON | LOG_NOTICE) << "Already connected to " << peer_id.toString() << endl;
            onFinish(false);
            return;
        }

        // send handshake and finish off
        sendHandshake(rh, pman->getTorrent().getPeerID());
        onFinish(true);
        // hand over connection
        pman->newConnection(std::move(sock), peer_id, supportedExtensions());
    } else {
        // if the handshake wasn't fully received just send our handshake
        sendHandshake(rh, pman->getTorrent().getPeerID());
    }
}
}

#include "moc_serverauthenticate.cpp"
