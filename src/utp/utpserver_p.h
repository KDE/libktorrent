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

#ifndef UTP_UTPSERVER_P_H
#define UTP_UTPSERVER_P_H

#include "connection.h"
#include "outputqueue.h"
#include "pollpipe.h"
#include "utpserver.h"
#include "utpsocket.h"
#include <QMap>
#include <QSocketNotifier>
#include <QTimer>
#include <ktorrent_export.h>
#include <net/poll.h>
#include <net/serversocket.h>
#include <net/socket.h>
#include <net/wakeuppipe.h>
#include <util/ptrmap.h>

namespace utp
{
class UTPServerThread;

/**
    Utility class used by UTPServer to make sure that ServerInterface::newConnection is called
    from the main thread and not from UTP thread (which is dangerous).
*/
class MainThreadCall : public QObject
{
    Q_OBJECT
public:
    MainThreadCall(UTPServer *server);
    ~MainThreadCall() override;

public Q_SLOTS:
    /**
        Calls UTPServer::handlePendingConnections, this should be executed in
        the main thread.
    */
    void handlePendingConnections();

private:
    UTPServer *server;
};

typedef bt::PtrMap<quint16, Connection>::iterator ConItr;

struct PollPipePair {
    PollPipe::Ptr read_pipe;
    PollPipe::Ptr write_pipe;

    PollPipePair();

    bool testRead(ConItr b, ConItr e);
    bool testWrite(ConItr b, ConItr e);
};

typedef bt::PtrMap<net::Poll *, PollPipePair>::iterator PollPipePairItr;
typedef QMap<quint16, Connection::Ptr>::iterator ConnectionMapItr;

class UTPServer::Private : public net::ServerSocket::DataHandler
{
public:
    Private(UTPServer *p);
    ~Private() override;

    bool bind(const net::Address &addr);
    void syn(const PacketParser &parser, bt::Buffer::Ptr buffer, const net::Address &addr);
    void reset(const Header *hdr);
    void wakeUpPollPipes(Connection::Ptr conn, bool readable, bool writeable);
    Connection::Ptr find(quint16 conn_id);
    void stop();
    void dataReceived(bt::Buffer::Ptr buffer, const net::Address &addr) override;
    void readyToWrite(net::ServerSocket *sock) override;

public:
    UTPServer *p;
    QList<net::ServerSocket::Ptr> sockets;
    bool running;
    QMap<quint16, Connection::Ptr> connections;
    UTPServerThread *utp_thread;
    QMutex mutex;
    bt::PtrMap<net::Poll *, PollPipePair> poll_pipes;
    bool create_sockets;
    bt::Uint8 tos;
    OutputQueue output_queue;
    QList<mse::EncryptedPacketSocket::Ptr> pending;
    QMutex pending_mutex;
    MainThreadCall *mtc;
    QList<Connection::WPtr> last_accepted;
    QTimer *timer;
};
}

#endif
