/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pipe.h"

#include "net/socket.h"
#include <fcntl.h>
#include <sys/types.h>
#include <util/functions.h>
#include <util/log.h>

#ifndef Q_OS_WIN
#include <sys/socket.h>
#include <unistd.h>
#else
#include <Winsock2.h>
#endif

using namespace Qt::Literals::StringLiterals;

namespace bt
{
#ifdef Q_OS_WIN
int socketpair(int sockets[2])
{
    sockets[0] = sockets[1] = -1;

    net::Socket sock(true, 4);
    if (!sock.bind(u"127.0.0.1"_s, 0, true))
        return -1;

    net::Address local_addr = sock.getSockName();
    net::Socket writer(true, 4);
    writer.setBlocking(false);
    writer.connectTo(local_addr);

    net::Address dummy;
    sockets[1] = sock.accept(dummy);
    if (sockets[1] < 0)
        return -1;

    if (!writer.connectSuccesFull()) {
        closesocket(sockets[1]);
        return -1;
    }

    sockets[0] = writer.take();
    Out(SYS_GEN | LOG_DEBUG) << "Created wakeup pipe" << endl;
    return 0;
}
#endif

Pipe::Pipe()
    : reader(-1)
    , writer(-1)
{
    int sockets[2];
#ifndef Q_OS_WIN
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0) {
        reader = sockets[1];
        writer = sockets[0];
        fcntl(writer, F_SETFL, O_NONBLOCK);
        fcntl(reader, F_SETFL, O_NONBLOCK);
    }
#else
    if (socketpair(sockets) == 0) {
        reader = sockets[1];
        writer = sockets[0];
    }
#endif
    else {
        Out(SYS_GEN | LOG_DEBUG) << "Cannot create wakeup pipe" << endl;
    }
}

Pipe::~Pipe()
{
#ifndef Q_OS_WIN
    if (reader >= 0)
        ::close(reader);
    if (writer >= 0)
        ::close(writer);
#else
    if (reader >= 0)
        ::closesocket(reader);
    if (writer >= 0)
        ::closesocket(writer);
#endif
}

int Pipe::read(Uint8 *buffer, int max_len)
{
#ifndef Q_OS_WIN
    return ::read(reader, buffer, max_len);
#else
    return ::recv(reader, (char *)buffer, max_len, 0);
#endif
}

int Pipe::write(const bt::Uint8 *data, int len)
{
#ifndef Q_OS_WIN
    return ::write(writer, data, len);
#else
    return ::send(writer, (char *)data, len, 0);
#endif
}

}
