/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "authenticationmonitor.h"
#include "authenticatebase.h"
#include "peerconnector.h"
#include <cmath>
#include <mse/encryptedpacketsocket.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
AuthenticationMonitor AuthenticationMonitor::self;

AuthenticationMonitor::AuthenticationMonitor()
{
}

AuthenticationMonitor::~AuthenticationMonitor()
{
}

void AuthenticationMonitor::clear()
{
    std::list<AuthenticateBase *>::iterator itr = auths.begin();
    while (itr != auths.end()) {
        AuthenticateBase *ab = *itr;
        ab->deleteLater();
        ++itr;
    }
    auths.clear();
}

void AuthenticationMonitor::shutdown()
{
    // No more active PeerConnector's allowed
    PeerConnector::setMaxActive(0);
    clear();
}

void AuthenticationMonitor::add(AuthenticateBase *s)
{
    auths.push_back(s);
}

void AuthenticationMonitor::remove(AuthenticateBase *s)
{
    auths.remove(s);
}

void AuthenticationMonitor::update()
{
    if (auths.size() == 0)
        return;

    reset();

    std::list<AuthenticateBase *>::iterator itr = auths.begin();
    while (itr != auths.end()) {
        AuthenticateBase *ab = *itr;
        if (!ab || ab->isFinished()) {
            if (ab)
                ab->deleteLater();

            itr = auths.erase(itr);
        } else {
            mse::EncryptedPacketSocket::Ptr socket = ab->getSocket();
            if (socket) {
                net::SocketDevice *dev = socket->socketDevice();
                if (dev) {
                    net::Poll::Mode m = socket->connecting() ? Poll::OUTPUT : Poll::INPUT;
                    dev->prepare(this, m);
                }
            }
            ++itr;
        }
    }

    if (poll(50)) {
        handleData();
    }
}

void AuthenticationMonitor::handleData()
{
    std::list<AuthenticateBase *>::iterator itr = auths.begin();
    while (itr != auths.end()) {
        AuthenticateBase *ab = *itr;
        if (!ab || ab->isFinished()) {
            if (ab)
                ab->deleteLater();
            itr = auths.erase(itr);
        } else {
            mse::EncryptedPacketSocket::Ptr socket = ab->getSocket();
            if (socket) {
                net::SocketDevice *dev = socket->socketDevice();
                bool r = dev && dev->ready(this, Poll::INPUT);
                bool w = dev && dev->ready(this, Poll::OUTPUT);
                if (r)
                    ab->onReadyRead();
                if (w)
                    ab->onReadyWrite();
            }
            ++itr;
        }
    }
}

}
