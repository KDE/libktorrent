/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "utpserver.h"
#include "utpserver_p.h"

#include <QCoreApplication>
#include <QEvent>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QTimer>

#include <cstdlib>

#ifndef Q_CC_MSVC
#include <sys/select.h>
#endif
#include "utpprotocol.h"
#include "utpserverthread.h"
#include "utpsocket.h"
#include <ctime>
#include <mse/encryptedpacketsocket.h>
#include <net/portlist.h>
#include <torrent/globals.h>
#include <util/constants.h>
#include <util/log.h>

#ifdef Q_OS_WIN
#include <util/win32.h>
#endif

using namespace bt;

namespace utp
{
MainThreadCall::MainThreadCall(UTPServer *server)
    : server(server)
{
}

MainThreadCall::~MainThreadCall()
{
}

void MainThreadCall::handlePendingConnections()
{
    server->handlePendingConnections();
}

///////////////////////////////////////////////////////////

UTPServer::Private::Private(UTPServer *p)
    : p(p)
    , running(false)
    , utp_thread(nullptr)
    , mutex()
    , create_sockets(true)
    , tos(0)
    , mtc(new MainThreadCall(p))
    , timer(new QTimer)
{
    QObject::connect(p, &UTPServer::handlePendingConnectionsDelayed, mtc, &MainThreadCall::handlePendingConnections, Qt::QueuedConnection);

    poll_pipes.setAutoDelete(true);
}

UTPServer::Private::~Private()
{
    if (running)
        stop();

    pending.clear();
    delete mtc;
    timer->deleteLater();
}

void UTPServer::Private::stop()
{
    QTimer::singleShot(0, timer, &QTimer::stop); // kill in its own thread
    running = false;
    if (utp_thread) {
        utp_thread->exit();
        utp_thread->wait();
        delete utp_thread;
        utp_thread = nullptr;
    }

    connections.clear();

    // Close the socket
    sockets.clear();
    Globals::instance().getPortList().removePort(port, net::UDP);
}

bool UTPServer::Private::bind(const net::Address &addr)
{
    net::ServerSocket::Ptr sock(new net::ServerSocket(this));
    if (!sock->bind(addr)) {
        return false;
    } else {
        Out(SYS_UTP | LOG_NOTICE) << "UTP: bound to " << addr.toString() << endl;
        sock->setTOS(tos);
        sock->setReadNotificationsEnabled(false);
        sock->setWriteNotificationsEnabled(false);
        sockets.append(sock);
        return true;
    }
}

void UTPServer::Private::syn(const PacketParser &parser, bt::Buffer::Ptr buffer, const net::Address &addr)
{
    const Header *hdr = parser.header();
    quint16 recv_conn_id = hdr->connection_id + 1;
    if (connections.contains(recv_conn_id)) {
        // Send a reset packet if the ID is in use
        Connection::Ptr conn(new Connection(recv_conn_id, Connection::INCOMING, addr, p));
        conn->setWeakPointer(conn);
        conn->sendReset();
    } else {
        Connection::Ptr conn(new Connection(recv_conn_id, Connection::INCOMING, addr, p));
        try {
            conn->setWeakPointer(conn);
            conn->handlePacket(parser, buffer);
            connections.insert(recv_conn_id, conn);
            if (create_sockets) {
                UTPSocket *utps = new UTPSocket(conn);
                mse::EncryptedPacketSocket::Ptr ss(new mse::EncryptedPacketSocket(utps));
                {
                    QMutexLocker lock(&pending_mutex);
                    pending.append(ss);
                }
                Q_EMIT p->handlePendingConnectionsDelayed();
            } else {
                last_accepted.append(conn);
                Q_EMIT p->accepted();
            }
        } catch (Connection::TransmissionError &err) {
            Out(SYS_UTP | LOG_NOTICE) << "UTP: " << err.location << endl;
            connections.remove(recv_conn_id);
        }
    }
}

void UTPServer::Private::reset(const utp::Header *hdr)
{
    Connection::Ptr c = find(hdr->connection_id);
    if (c) {
        c->reset();
    }
}

Connection::Ptr UTPServer::Private::find(quint16 conn_id)
{
    ConnectionMapItr i = connections.find(conn_id);
    if (i != connections.end())
        return i.value();
    else
        return Connection::Ptr();
}

void UTPServer::Private::wakeUpPollPipes(utp::Connection::Ptr conn, bool readable, bool writeable)
{
    QMutexLocker lock(&mutex);
    for (PollPipePairItr itr = poll_pipes.begin(); itr != poll_pipes.end(); ++itr) {
        PollPipePair *pp = itr->second;
        if (readable && pp->read_pipe->polling(conn->receiveConnectionID()))
            itr->second->read_pipe->wakeUp();

        if (writeable && pp->write_pipe->polling(conn->receiveConnectionID()))
            itr->second->write_pipe->wakeUp();
    }
}

void UTPServer::Private::dataReceived(bt::Buffer::Ptr buffer, const net::Address &addr)
{
    QMutexLocker lock(&mutex);
    // Out(SYS_UTP|LOG_NOTICE) << "UTP: received " << ba << " bytes packet from " << addr.toString() << endl;
    try {
        if (buffer->size() >= utp::Header::size()) { // discard packets which are to small
            p->handlePacket(buffer, addr);
        }
    } catch (utp::Connection::TransmissionError &err) {
        Out(SYS_UTP | LOG_NOTICE) << "UTP: " << err.location << endl;
    }
}

void UTPServer::Private::readyToWrite(net::ServerSocket *sock)
{
    output_queue.send(sock);
}

///////////////////////////////////////////////////////////

UTPServer::UTPServer(QObject *parent)
    : ServerInterface(parent)
    , d(std::make_unique<Private>(this))

{
    connect(d->timer, &QTimer::timeout, this, &UTPServer::checkTimeouts);
}

UTPServer::~UTPServer()
{
}

void UTPServer::handlePendingConnections()
{
    // This should be called from the main thread
    QList<mse::EncryptedPacketSocket::Ptr> p;
    {
        QMutexLocker lock(&d->pending_mutex);
        // Copy the pending list and clear it before using it's contents to avoid a deadlock
        p = d->pending;
        d->pending.clear();
    }

    for (const mse::EncryptedPacketSocket::Ptr &s : std::as_const(p)) {
        newConnection(s);
    }
}

bool UTPServer::changePort(bt::Uint16 p)
{
    if (d->sockets.count() > 0 && port == p)
        return true;

    Globals::instance().getPortList().removePort(port, net::UDP);
    d->sockets.clear();

    const QStringList possible = bindAddresses();
    for (const QString &addr : possible) {
        d->bind(net::Address(addr, p));
    }

    if (d->sockets.count() == 0) {
        // Try any addresses if previous binds failed
        d->bind(net::Address(QHostAddress(QHostAddress::AnyIPv6).toString(), p));
        d->bind(net::Address(QHostAddress(QHostAddress::Any).toString(), p));
    }

    if (d->sockets.count()) {
        Globals::instance().getPortList().addNewPort(p, net::UDP, true);
        return true;
    } else
        return false;
}

void UTPServer::setTOS(Uint8 type_of_service)
{
    d->tos = type_of_service;
    for (net::ServerSocket::Ptr sock : std::as_const(d->sockets))
        sock->setTOS(d->tos);
}

void UTPServer::threadStarted()
{
    d->timer->start(500);
    for (net::ServerSocket::Ptr sock : std::as_const(d->sockets)) {
        sock->setReadNotificationsEnabled(true);
    }
}

#if 0
static void Dump(const QByteArray & data, const net::Address& addr)
{
    Out(SYS_UTP | LOG_DEBUG) << QString("Received packet from %1 (%2 bytes)").arg(addr.toString()).arg(data.size()) << endl;
    const bt::Uint8* pkt = (const bt::Uint8*)data.data();

    QString line;
    for (int i = 0; i < data.size(); i++) {
        if (i > 0 && i % 32 == 0) {
            Out(SYS_UTP | LOG_DEBUG) << line << endl;
            line = QString();
        }

        uint val = pkt[i];
        line += QString("%1").arg(val, 2, 16, QChar('0'));
        if (i + 1 % 4)
            line += ' ';
    }
    Out(SYS_UTP | LOG_DEBUG) << line << endl;
}

static void DumpPacket(const Header & hdr)
{
    Out(SYS_UTP | LOG_NOTICE) << "==============================================" << endl;
    Out(SYS_UTP | LOG_NOTICE) << "UTP: Packet Header: " << endl;
    Out(SYS_UTP | LOG_NOTICE) << "type:                              " << TypeToString(hdr.type) << endl;
    Out(SYS_UTP | LOG_NOTICE) << "version:                           " << hdr.version << endl;
    Out(SYS_UTP | LOG_NOTICE) << "extension:                         " << hdr.extension << endl;
    Out(SYS_UTP | LOG_NOTICE) << "connection_id:                     " << hdr.connection_id << endl;
    Out(SYS_UTP | LOG_NOTICE) << "timestamp_microseconds:            " << hdr.timestamp_microseconds << endl;
    Out(SYS_UTP | LOG_NOTICE) << "timestamp_difference_microseconds: " << hdr.timestamp_difference_microseconds << endl;
    Out(SYS_UTP | LOG_NOTICE) << "wnd_size:                          " << hdr.wnd_size << endl;
    Out(SYS_UTP | LOG_NOTICE) << "seq_nr:                            " << hdr.seq_nr << endl;
    Out(SYS_UTP | LOG_NOTICE) << "ack_nr:                            " << hdr.ack_nr << endl;
    Out(SYS_UTP | LOG_NOTICE) << "==============================================" << endl;
}
#endif

void UTPServer::handlePacket(bt::Buffer::Ptr buffer, const net::Address &addr)
{
    PacketParser parser(buffer->get(), buffer->size());
    if (!parser.parse())
        return;

    const Header *hdr = parser.header();
    // Dump(packet,addr);
    // DumpPacket(*hdr);
    Connection::Ptr c;
    switch (hdr->type) {
    case ST_DATA:
    case ST_FIN:
    case ST_STATE:
        try {
            c = d->find(hdr->connection_id);
            if (c && c->handlePacket(parser, buffer) == CS_CLOSED) {
                d->connections.remove(c->receiveConnectionID());
            }
        } catch (Connection::TransmissionError &err) {
            Out(SYS_UTP | LOG_NOTICE) << "UTP: " << err.location << endl;
            if (c)
                c->close();
        }
        break;
    case ST_RESET:
        d->reset(hdr);
        break;
    case ST_SYN:
        d->syn(parser, buffer, addr);
        break;
    }
}

bool UTPServer::sendTo(utp::Connection::Ptr conn, const PacketBuffer &packet)
{
    if (d->output_queue.add(packet, conn) == 1) {
        // If there is only one packet queued,
        // We need to enable the write notifiers, use the event queue to do this
        // if we are not in the utp thread
        if (QThread::currentThread() != d->utp_thread) {
            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
        } else {
            for (net::ServerSocket::Ptr sock : std::as_const(d->sockets))
                sock->setWriteNotificationsEnabled(true);
        }
    }
    return true;
}

void UTPServer::customEvent(QEvent *ev)
{
    if (ev->type() == QEvent::User) {
        for (net::ServerSocket::Ptr sock : std::as_const(d->sockets))
            sock->setWriteNotificationsEnabled(true);
    }
}

Connection::WPtr UTPServer::connectTo(const net::Address &addr)
{
    if (d->sockets.isEmpty() || addr.port() == 0)
        return Connection::WPtr();

    QMutexLocker lock(&d->mutex);
    quint16 recv_conn_id = QRandomGenerator::global()->bounded(32535);
    while (d->connections.contains(recv_conn_id))
        recv_conn_id = QRandomGenerator::global()->bounded(32535);

    Connection::Ptr conn(new Connection(recv_conn_id, Connection::OUTGOING, addr, this));
    conn->setWeakPointer(conn);
    conn->moveToThread(d->utp_thread);
    d->connections.insert(recv_conn_id, conn);
    try {
        conn->startConnecting();
        return conn;
    } catch (Connection::TransmissionError &err) {
        d->connections.remove(recv_conn_id);
        return Connection::WPtr();
    }
}

void UTPServer::stop()
{
    d->stop();
    PacketBuffer::clearPool();
}

void UTPServer::start()
{
    if (!d->utp_thread) {
        d->utp_thread = new UTPServerThread(this);
        for (const net::ServerSocket::Ptr &sock : std::as_const(d->sockets))
            sock->moveToThread(d->utp_thread);
        d->timer->moveToThread(d->utp_thread);
        d->utp_thread->start();
    }
}

void UTPServer::preparePolling(net::Poll *p, net::Poll::Mode mode, utp::Connection::Ptr &conn)
{
    QMutexLocker lock(&d->mutex);
    PollPipePair *pair = d->poll_pipes.find(p);
    if (!pair) {
        pair = new PollPipePair();
        d->poll_pipes.insert(p, pair);
    }

    if (mode == net::Poll::INPUT) {
        if (pair->read_pipe->wokenUp())
            return;

        if (conn->bytesAvailable() > 0 || conn->connectionState() == CS_CLOSED)
            pair->read_pipe->wakeUp();
        pair->read_pipe->prepare(p, conn->receiveConnectionID(), pair->read_pipe);
    } else {
        if (pair->write_pipe->wokenUp())
            return;

        if (conn->isWriteable())
            pair->write_pipe->wakeUp();
        pair->write_pipe->prepare(p, conn->receiveConnectionID(), pair->write_pipe);
    }
}

void UTPServer::stateChanged(utp::Connection::Ptr conn, bool readable, bool writeable)
{
    d->wakeUpPollPipes(conn, readable, writeable);
}

void UTPServer::setCreateSockets(bool on)
{
    d->create_sockets = on;
}

Connection::WPtr UTPServer::acceptedConnection()
{
    if (d->last_accepted.isEmpty())
        return Connection::WPtr();
    else
        return d->last_accepted.takeFirst();
}

void UTPServer::closed(Connection::Ptr conn)
{
    Q_UNUSED(conn);
    QTimer::singleShot(0, this, &UTPServer::cleanup);
}

void UTPServer::cleanup()
{
    QMutexLocker lock(&d->mutex);
    QMap<quint16, Connection::Ptr>::iterator i = d->connections.begin();
    while (i != d->connections.end()) {
        if (i.value()->connectionState() == CS_CLOSED) {
            i = d->connections.erase(i);
        } else
            ++i;
    }
}

void UTPServer::checkTimeouts()
{
    QMutexLocker lock(&d->mutex);

    TimeValue now;
    QMap<quint16, Connection::Ptr>::iterator itr = d->connections.begin();
    while (itr != d->connections.end()) {
        itr.value()->checkTimeout(now);
        ++itr;
    }
}

///////////////////////////////////////////////////////

PollPipePair::PollPipePair()
    : read_pipe(new PollPipe(net::Poll::INPUT))
    , write_pipe(new PollPipe(net::Poll::OUTPUT))
{
}
}

#include "moc_utpserver.cpp"
#include "moc_utpserver_p.cpp"
