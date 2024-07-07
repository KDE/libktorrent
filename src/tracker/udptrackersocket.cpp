/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "udptrackersocket.h"
#include <QHostAddress>
#include <klocalizedstring.h>
#include <net/portlist.h>
#include <net/serversocket.h>
#include <net/socket.h>
#include <stdlib.h>
#include <time.h>
#include <torrent/globals.h>
#include <util/array.h>
#include <util/functions.h>
#include <util/log.h>

#ifdef ERROR
#undef ERROR
#endif

namespace bt
{
Uint16 UDPTrackerSocket::port = 4444;

class UDPTrackerSocket::Private : public net::ServerSocket::DataHandler
{
public:
    Private(UDPTrackerSocket *p)
        : p(p)
    {
    }

    ~Private() override
    {
    }

    void listen(const QString &ip, Uint16 port)
    {
        net::ServerSocket::Ptr sock(new net::ServerSocket(this));
        if (sock->bind(ip, port))
            sockets.append(sock);
    }

    bool send(const Uint8 *buf, int size, const net::Address &addr)
    {
        for (net::ServerSocket::Ptr sock : std::as_const(sockets))
            if (sock->sendTo(buf, size, addr) == size)
                return true;

        return false;
    }

    void dataReceived(bt::Buffer::Ptr buffer, const net::Address &addr) override
    {
        Q_UNUSED(addr);
        if (buffer->size() < 4)
            return;

        Uint32 type = ReadUint32(buffer->get(), 0);
        switch (type) {
        case CONNECT:
            p->handleConnect(buffer);
            break;
        case ANNOUNCE:
            p->handleAnnounce(buffer);
            break;
        case ERROR:
            p->handleError(buffer);
            break;
        case SCRAPE:
            p->handleScrape(buffer);
            break;
        }
    }

    void readyToWrite(net::ServerSocket *sock) override
    {
        Q_UNUSED(sock);
    }

    QList<net::ServerSocket::Ptr> sockets;
    QMap<Int32, Action> transactions;
    UDPTrackerSocket *p;
};

UDPTrackerSocket::UDPTrackerSocket()
    : d(new Private(this))
{
    if (port == 0)
        port = 4444;

    const QStringList ips = NetworkInterfaceIPAddresses(NetworkInterface());
    for (const QString &ip : ips)
        d->listen(ip, port);

    if (d->sockets.count() == 0) {
        // Try all addresses if the previous listen calls all failed
        d->listen(QHostAddress(QHostAddress::AnyIPv6).toString(), port);
        d->listen(QHostAddress(QHostAddress::Any).toString(), port);
    }

    if (d->sockets.count() == 0) {
        Out(SYS_TRK | LOG_IMPORTANT) << QString("Cannot bind to udp port %1").arg(port) << endl;
    } else {
        Globals::instance().getPortList().addNewPort(port, net::UDP, true);
    }
}

UDPTrackerSocket::~UDPTrackerSocket()
{
    Globals::instance().getPortList().removePort(port, net::UDP);
    delete d;
}

void UDPTrackerSocket::sendConnect(Int32 tid, const net::Address &addr)
{
    Int64 cid = 0x41727101980LL;
    Uint8 buf[16];

    WriteInt64(buf, 0, cid);
    WriteInt32(buf, 8, CONNECT);
    WriteInt32(buf, 12, tid);

    d->send(buf, 16, addr);
    d->transactions.insert(tid, CONNECT);
}

void UDPTrackerSocket::sendAnnounce(Int32 tid, const Uint8 *data, const net::Address &addr)
{
    d->send(data, 98, addr);
    d->transactions.insert(tid, ANNOUNCE);
}

void UDPTrackerSocket::sendScrape(Int32 tid, const bt::Uint8 *data, const net::Address &addr)
{
    d->send(data, 36, addr);
    d->transactions.insert(tid, SCRAPE);
}

void UDPTrackerSocket::cancelTransaction(Int32 tid)
{
    d->transactions.remove(tid);
}

void UDPTrackerSocket::handleConnect(bt::Buffer::Ptr buf)
{
    if (buf->size() < 12)
        return;

    // Read the transaction_id and check it
    Int32 tid = ReadInt32(buf->get(), 4);
    QMap<Int32, Action>::iterator i = d->transactions.find(tid);
    // if we can't find the transaction, just return
    if (i == d->transactions.end())
        return;

    // check whether the transaction is a CONNECT
    if (i.value() != CONNECT) {
        d->transactions.erase(i);
        Q_EMIT error(tid, QString());
        return;
    }

    // everything ok, emit signal
    d->transactions.erase(i);
    Q_EMIT connectReceived(tid, ReadInt64(buf->get(), 8));
}

void UDPTrackerSocket::handleAnnounce(bt::Buffer::Ptr buf)
{
    if (buf->size() < 4)
        return;

    // Read the transaction_id and check it
    Int32 tid = ReadInt32(buf->get(), 4);
    QMap<Int32, Action>::iterator i = d->transactions.find(tid);
    // if we can't find the transaction, just return
    if (i == d->transactions.end() || buf->size() < 20)
        return;

    // check whether the transaction is a ANNOUNCE
    if (i.value() != ANNOUNCE) {
        d->transactions.erase(i);
        Q_EMIT error(tid, QString());
        return;
    }

    // everything ok, emit signal
    d->transactions.erase(i);
    Q_EMIT announceReceived(tid, buf->get(), buf->size());
}

void UDPTrackerSocket::handleError(bt::Buffer::Ptr buf)
{
    if (buf->size() < 4)
        return;

    // Read the transaction_id and check it
    Int32 tid = ReadInt32(buf->get(), 4);
    QMap<Int32, Action>::iterator it = d->transactions.find(tid);
    // if we can't find the transaction, just return
    if (it == d->transactions.end())
        return;

    // extract error message
    d->transactions.erase(it);
    QString msg;
    for (Uint32 i = 8; i < buf->size(); i++)
        msg += (char)buf->get()[i];

    // emit signal
    Q_EMIT error(tid, msg);
}

void UDPTrackerSocket::handleScrape(bt::Buffer::Ptr buf)
{
    if (buf->size() < 4)
        return;

    // Read the transaction_id and check it
    Int32 tid = ReadInt32((Uint8 *)buf.data(), 4);
    QMap<Int32, Action>::iterator i = d->transactions.find(tid);
    // if we can't find the transaction, just return
    if (i == d->transactions.end())
        return;

    // check whether the transaction is a SCRAPE
    if (i.value() != SCRAPE) {
        d->transactions.erase(i);
        Q_EMIT error(tid, QString());
        return;
    }

    // everything ok, emit signal
    d->transactions.erase(i);
    Q_EMIT scrapeReceived(tid, buf->get(), buf->size());
}

Int32 UDPTrackerSocket::newTransactionID()
{
    Int32 transaction_id = rand() * time(nullptr);
    while (d->transactions.contains(transaction_id))
        transaction_id++;
    return transaction_id;
}

void UDPTrackerSocket::setPort(Uint16 p)
{
    port = p;
}

Uint16 UDPTrackerSocket::getPort()
{
    return port;
}
}

#include "moc_udptrackersocket.cpp"
