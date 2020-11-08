/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
#include "server.h"
#include <QStringList>
#include <QHostAddress>
#include <qsocketnotifier.h>
#include <net/socket.h>
#include <mse/encryptedpacketsocket.h>
#include <util/sha1hash.h>
#include <util/log.h>
#include <util/functions.h>
#include <net/portlist.h>
#include <mse/encryptedserverauthenticate.h>
#include <peer/peermanager.h>
#include <peer/serverauthenticate.h>
#include <peer/authenticationmonitor.h>
#include <peer/accessmanager.h>

#include "globals.h"
#include "torrent.h"
#include <net/serversocket.h>


namespace bt
{
typedef QSharedPointer<net::ServerSocket> ServerSocketPtr;

class Server::Private : public net::ServerSocket::ConnectionHandler
{
public:
    Private(Server* p) : p(p)
    {
    }

    ~Private() override
    {
    }

    void reset()
    {
        sockets.clear();
    }

    void newConnection(int fd, const net::Address & addr) override
    {
        mse::EncryptedPacketSocket::Ptr s(new mse::EncryptedPacketSocket(fd, addr.ipVersion()));
        p->newConnection(s);
    }

    void add(const QString & ip, bt::Uint16 port)
    {
        ServerSocketPtr sock(new net::ServerSocket(this));
        if (sock->bind(ip, port)) {
            sockets.append(sock);
        }
    }


    Server* p;
    QList<ServerSocketPtr> sockets;
};

Server::Server() : d(new Private(this))
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
    for (const QString & addr : possible) {
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

