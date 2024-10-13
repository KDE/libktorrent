/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <util/log.h>
#include <utp/connection.h>
#include <utp/utpserver.h>

using namespace utp;
using namespace Qt::Literals::StringLiterals;

class ConnectionTest : public QEventLoop, public Transmitter
{
public:
    ConnectionTest(QObject *parent = nullptr)
        : QEventLoop(parent)
        , remote(u"127.0.0.1"_s, 50000)
    {
    }

    bool sendTo(Connection::Ptr conn, const PacketBuffer &packet) override
    {
        sent_packets.append(packet);
        Q_UNUSED(conn)
        return true;
    }

    void stateChanged(Connection::Ptr conn, bool readable, bool writeable) override
    {
        Q_UNUSED(conn)
        Q_UNUSED(readable)
        Q_UNUSED(writeable)
    }

    void closed(Connection::Ptr conn) override
    {
        Q_UNUSED(conn)
    }

    bt::Buffer::Ptr buildPacket(bt::Uint32 type, bt::Uint32 recv_conn_id, bt::Uint32 send_conn_id, bt::Uint16 seq_nr, bt::Uint16 ack_nr)
    {
        TimeValue tv;
        bt::Buffer::Ptr packet = pool->get(Header::size());
        Header hdr;
        hdr.version = 1;
        hdr.type = type;
        hdr.extension = 0;
        hdr.connection_id = type == ST_SYN ? recv_conn_id : send_conn_id;
        hdr.timestamp_microseconds = tv.microseconds;
        hdr.timestamp_difference_microseconds = 0;
        hdr.wnd_size = 6666;
        hdr.seq_nr = seq_nr;
        hdr.ack_nr = ack_nr;
        hdr.write(packet->get());
        return packet;
    }

private:
    void initTestCase()
    {
        bt::InitLog(u"connectiontest.log"_s);
        pool = bt::BufferPool::Ptr(new bt::BufferPool());
        pool->setWeakPointer(pool.toWeakRef());
    }

    void cleanupTestCase()
    {
        pool.clear();
    }

    void init()
    {
        sent_packets.clear();
    }

    void testConnID()
    {
        bt::Uint32 conn_id = 666;
        Connection conn(conn_id, utp::Connection::INCOMING, remote, this);
        QVERIFY(conn.connectionStats().recv_connection_id == conn_id);
        QVERIFY(conn.connectionStats().send_connection_id == conn_id - 1);

        Connection conn2(conn_id, utp::Connection::OUTGOING, remote, this);
        QVERIFY(conn2.connectionStats().recv_connection_id == conn_id);
        QVERIFY(conn2.connectionStats().send_connection_id == conn_id + 1);
    }

    void testOutgoingConnectionSetup()
    {
        bt::Uint32 conn_id = 666;
        Connection conn(conn_id, utp::Connection::OUTGOING, remote, this);
        conn.startConnecting();
        const Connection::Stats &s = conn.connectionStats();
        QVERIFY(s.state == utp::CS_SYN_SENT);
        QVERIFY(s.seq_nr == 2);

        bt::Buffer::Ptr pkt = buildPacket(ST_STATE, conn_id, conn_id + 1, 1, 1);
        PacketParser pp(pkt->get(), pkt->size());
        QVERIFY(pp.parse());
        conn.handlePacket(pp, pkt);
        QVERIFY(s.state == CS_CONNECTED);
        QVERIFY(sent_packets.count() == 1);
    }

    void testIncomingConnectionSetup()
    {
        bt::Uint32 conn_id = 666;
        Connection conn(conn_id, utp::Connection::INCOMING, remote, this);
        const Connection::Stats &s = conn.connectionStats();

        bt::Buffer::Ptr pkt = buildPacket(ST_SYN, conn_id - 1, conn_id, 1, 1);
        PacketParser pp(pkt->get(), pkt->size());
        conn.handlePacket(pp, pkt);
        QVERIFY(s.state == CS_CONNECTED);
    }

private:
    net::Address remote;
    QList<PacketBuffer> sent_packets;
    bt::BufferPool::Ptr pool;
};

QTEST_MAIN(ConnectionTest)
