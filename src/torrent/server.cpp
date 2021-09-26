/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "server.h"
#include <QHostAddress>
#include <QStringList>
#include <mse/encryptedpacketsocket.h>
#include <mse/encryptedserverauthenticate.h>
#include <net/portlist.h>
#include <net/socket.h>
#include <peer/accessmanager.h>
#include <peer/authenticationmonitor.h>
#include <peer/peermanager.h>
#include <peer/serverauthenticate.h>
#include <qsocketnotifier.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/sha1hash.h>

#include "globals.h"
#include "torrent.h"
#include <net/serversocket.h>

namespace bt
{
typedef QSharedPointer<net::ServerSocket> ServerSocketPtr;

class Server::Private : public net::ServerSocket::ConnectionHandler
{
public:
    Private(Server *p)
        : p(p)
    {
    }

    ~Private() override
    {
    }

    void reset()
    {
        sockets.clear();
    }

    void newConnection(int fd, const net::Address &addr) override
    {
        mse::EncryptedPacketSocket::Ptr s(new mse::EncryptedPacketSocket(fd, addr.ipVersion()));
        p->newConnection(s);
    }

    void add(const QString &ip, bt::Uint16 port)
    {
        ServerSocketPtr sock(new net::ServerSocket(this));
        if (sock->bind(ip, port)) {
            sockets.append(sock);
        }
    }

    Server *p;
    QList<ServerSocketPtr> sockets;
};

Server::Server()
    : d(new Private(this))
{
}

Server::~Server()
{
    Globals::instance().getPortList().removePort(port, net::TCP);
    delete d;
}

bool Server::changePort(Uint16 p)
{
    if (d->sockets.count() > 0 && p == port)
        return true;

    Globals::instance().getPortList().removePort(port, net::TCP);
    d->reset();

    const QStringList possible = bindAddresses();
    for (const QString &addr : possible) {
        d->add(addr, p);
    }

    if (d->sockets.count() == 0) {
        // Try any addresses if previous binds failed
        d->add(QHostAddress(QHostAddress::AnyIPv6).toString(), p);
        d->add(QHostAddress(QHostAddress::Any).toString(), p);
    }

    if (d->sockets.count()) {
        Globals::instance().getPortList().addNewPort(p, net::TCP, true);
        return true;
    } else
        return false;
}

}
