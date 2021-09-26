/*
    SPDX-FileCopyrightText: 2009 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "utpsocket.h"
#include "connection.h"
#include "utpserver.h"
#include <torrent/globals.h>

namespace utp
{
UTPSocket::UTPSocket()
    : net::SocketDevice(bt::UTP)
    , blocking(true)
    , polled_for_reading(false)
    , polled_for_writing(false)
{
}

UTPSocket::UTPSocket(Connection::WPtr conn)
    : net::SocketDevice(bt::UTP)
    , conn(conn)
    , blocking(true)
    , polled_for_reading(false)
    , polled_for_writing(false)
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (ptr) {
        setRemoteAddress(ptr->remoteAddress());
        ptr->setBlocking(blocking);
        m_state = CONNECTED;
    }
}

UTPSocket::~UTPSocket()
{
    close();
    reset();
}

bt::Uint32 UTPSocket::bytesAvailable() const
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (ptr)
        return ptr->bytesAvailable();
    else
        return 0;
}

void UTPSocket::close()
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (ptr) {
        try {
            ptr->close();
        } catch (Connection::TransmissionError) {
            reset();
        }
    }
}

bool UTPSocket::connectSuccesFull()
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (ptr && ptr->connectionState() == CS_CONNECTED) {
        setRemoteAddress(ptr->remoteAddress());
        m_state = CONNECTED;
        return true;
    } else
        return false;
}

bool UTPSocket::connectTo(const net::Address &addr)
{
    if (!bt::Globals::instance().isUTPEnabled())
        return false;

    UTPServer &srv = bt::Globals::instance().getUTPServer();
    reset();

    conn = srv.connectTo(addr);
    Connection::Ptr ptr = conn.toStrongRef();
    if (!ptr)
        return false;

    m_state = CONNECTING;
    ptr->setBlocking(blocking);
    if (blocking) {
        bool ret = ptr->waitUntilConnected();
        if (ret)
            m_state = CONNECTED;

        return ret;
    }

    return ptr->connectionState() == CS_CONNECTED;
}

int UTPSocket::fd() const
{
    return -1;
}

const net::Address &UTPSocket::getPeerName() const
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (remote_addr_override)
        return addr;
    else if (ptr)
        return ptr->remoteAddress();
    else {
        static net::Address null;
        return null;
    }
}

net::Address UTPSocket::getSockName() const
{
    return net::Address();
}

bool UTPSocket::ok() const
{
    Connection::Ptr ptr = conn.toStrongRef();
    return ptr && ptr->connectionState() != CS_CLOSED;
}

int UTPSocket::recv(bt::Uint8 *buf, int max_len)
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (!ptr || ptr->connectionState() == CS_CLOSED)
        return 0;

    try {
        if (ptr->bytesAvailable() == 0) {
            if (blocking) {
                if (ptr->waitForData())
                    return ptr->recv(buf, max_len);
                else
                    return 0; // connection should be closed now
            } else
                return -1; // No data ready and not blocking so return -1
        } else
            return ptr->recv(buf, max_len);
    } catch (Connection::TransmissionError &err) {
        close();
        return -1;
    }
}

void UTPSocket::reset()
{
    conn.clear();
}

int UTPSocket::send(const bt::Uint8 *buf, int len)
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (!ptr)
        return -1;

    try {
        return ptr->send(buf, len);
    } catch (Connection::TransmissionError &err) {
        close();
        return -1;
    }
}

void UTPSocket::setBlocking(bool on)
{
    blocking = on;
    Connection::Ptr ptr = conn.toStrongRef();
    if (ptr)
        ptr->setBlocking(on);
}

bool UTPSocket::setTOS(unsigned char type_of_service)
{
    Q_UNUSED(type_of_service);
    return true;
}

void UTPSocket::prepare(net::Poll *p, net::Poll::Mode mode)
{
    Connection::Ptr ptr = conn.toStrongRef();
    if (ptr && ptr->connectionState() != CS_CLOSED) {
        UTPServer &srv = bt::Globals::instance().getUTPServer();
        srv.preparePolling(p, mode, ptr);
        if (mode == net::Poll::OUTPUT)
            polled_for_writing = true;
        else
            polled_for_reading = true;
    }
}

bool UTPSocket::ready(const net::Poll *p, net::Poll::Mode mode) const
{
    Q_UNUSED(p);
    Connection::Ptr ptr = conn.toStrongRef();
    if (!ptr)
        return false;

    if (mode == net::Poll::OUTPUT) {
        if (polled_for_writing) {
            polled_for_writing = false;
            return ptr->isWriteable();
        }
    } else {
        if (polled_for_reading) {
            polled_for_reading = false;
            return bytesAvailable() > 0 || ptr->connectionState() == CS_CLOSED;
        }
    }

    return false;
}

}
