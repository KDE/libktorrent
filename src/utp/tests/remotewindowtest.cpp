/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <util/functions.h>
#include <util/log.h>
#include <utp/connection.h>
#include <utp/remotewindow.h>

using namespace utp;
using namespace Qt::Literals::StringLiterals;

class RemoteWindowTest : public QObject, public utp::Retransmitter
{
    Q_OBJECT
public:
    QSet<bt::Uint16> retransmit_seq_nr;
    bool update_rtt_called;
    bool retransmit_ok;

    RemoteWindowTest(QObject *parent = nullptr)
        : QObject(parent)
        , update_rtt_called(false)
        , retransmit_ok(false)
    {
    }

    void updateRTT(const Header *hdr, bt::Uint32 packet_rtt, bt::Uint32 packet_size) override
    {
        Q_UNUSED(hdr);
        Q_UNUSED(packet_rtt);
        Q_UNUSED(packet_size);
        update_rtt_called = true;
    }

    void retransmit(PacketBuffer & /*packet*/, bt::Uint16 p_seq_nr) override
    {
        bt::Out(SYS_UTP | LOG_NOTICE) << "retransmit " << p_seq_nr << bt::endl;
        retransmit_ok = retransmit_seq_nr.contains(p_seq_nr);
    }

    void reset()
    {
        retransmit_seq_nr.clear();
        update_rtt_called = false;
        retransmit_ok = false;
    }

    [[nodiscard]] bt::Uint32 currentTimeout() const override
    {
        return 1000;
    }

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"remotewindowtest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void init()
    {
        reset();
    }

    void testNormalUsage()
    {
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        wnd.addPacket(pkt, 1, bt::Now());
        QVERIFY(!wnd.allPacketsAcked());
        QVERIFY(wnd.numUnackedPackets() == 1);

        wnd.addPacket(pkt, 2, bt::Now());
        QVERIFY(wnd.numUnackedPackets() == 2);

        wnd.addPacket(pkt, 3, bt::Now());
        QVERIFY(wnd.numUnackedPackets() == 3);

        Header hdr;
        hdr.ack_nr = 1;
        hdr.wnd_size = 5000;
        wnd.packetReceived(&hdr, nullptr, this);
        QVERIFY(wnd.numUnackedPackets() == 2);
        QVERIFY(update_rtt_called);

        reset();
        hdr.ack_nr = 2;
        wnd.packetReceived(&hdr, nullptr, this);
        QVERIFY(wnd.numUnackedPackets() == 1);
        QVERIFY(update_rtt_called);

        reset();
        hdr.ack_nr = 3;
        wnd.packetReceived(&hdr, nullptr, this);
        QVERIFY(wnd.numUnackedPackets() == 0);
        QVERIFY(update_rtt_called);
    }

    void testMultipleAcks()
    {
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        wnd.addPacket(pkt, 1, bt::Now());
        QVERIFY(!wnd.allPacketsAcked());
        QVERIFY(wnd.numUnackedPackets() == 1);

        wnd.addPacket(pkt, 2, bt::Now());
        QVERIFY(wnd.numUnackedPackets() == 2);

        wnd.addPacket(pkt, 3, bt::Now());
        QVERIFY(wnd.numUnackedPackets() == 3);

        Header hdr;
        hdr.ack_nr = 3;
        hdr.wnd_size = 5000;
        wnd.packetReceived(&hdr, nullptr, this);
        QVERIFY(wnd.numUnackedPackets() == 0);
        QVERIFY(update_rtt_called);
    }

    void testSelectiveAck()
    {
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        wnd.addPacket(pkt, 1, bt::Now());
        QVERIFY(!wnd.allPacketsAcked());
        QVERIFY(wnd.numUnackedPackets() == 1);

        wnd.addPacket(pkt, 2, bt::Now());
        QVERIFY(wnd.numUnackedPackets() == 2);

        wnd.addPacket(pkt, 3, bt::Now());
        QVERIFY(wnd.numUnackedPackets() == 3);

        // Selectively ack 3
        bt::Uint8 sack_data[6];
        memset(sack_data, 0, 6);
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        Ack(&sack, 3);

        Header hdr;
        hdr.ack_nr = 0;
        hdr.wnd_size = 5000;
        wnd.packetReceived(&hdr, &sack, this);
        QVERIFY(wnd.numUnackedPackets() == 2);
        QVERIFY(update_rtt_called);

        reset();

        // Ack the rest
        hdr.ack_nr = 3;
        hdr.wnd_size = 5000;
        wnd.packetReceived(&hdr, nullptr, this);
        QVERIFY(wnd.numUnackedPackets() == 0);
        QVERIFY(update_rtt_called);
    }

    void testRetransmits()
    {
        reset();
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        for (bt::Uint32 i = 0; i < 4; i++) {
            wnd.addPacket(pkt, i + 1, bt::Now());
            QVERIFY(!wnd.allPacketsAcked());
            QVERIFY(wnd.numUnackedPackets() == i + 1);
        }

        // Selectively ack the last 3 packets
        bt::Uint8 sack_data[6];
        memset(sack_data, 0, 6);
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        Ack(&sack, 2);
        Ack(&sack, 3);
        Ack(&sack, 4);
        Header hdr;
        hdr.ack_nr = 0;
        hdr.wnd_size = 5000;
        retransmit_seq_nr.insert(1);
        wnd.packetReceived(&hdr, &sack, this);
        QVERIFY(wnd.numUnackedPackets() == 1);
        QVERIFY(update_rtt_called);
        QVERIFY(retransmit_ok);
    }

    void testMultipleRetransmits()
    {
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        for (bt::Uint32 i = 0; i < 4; i++) {
            wnd.addPacket(pkt, i + 1, bt::Now());
            QVERIFY(!wnd.allPacketsAcked());
            QVERIFY(wnd.numUnackedPackets() == i + 1);
        }

        // Selectively ack the last 3 packets
        bt::Uint8 sack_data[6];
        memset(sack_data, 0, 6);
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        Ack(&sack, 2);
        Ack(&sack, 3);
        Ack(&sack, 4);
        Header hdr;
        hdr.ack_nr = 0;
        hdr.wnd_size = 5000;
        retransmit_seq_nr.insert(1);
        wnd.packetReceived(&hdr, &sack, this);
        QVERIFY(wnd.numUnackedPackets() == 1);
        QVERIFY(update_rtt_called);
        QVERIFY(retransmit_ok);
    }

    void testMultipleRetransmits2()
    {
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        for (bt::Uint32 i = 0; i < 10; i++) {
            wnd.addPacket(pkt, i + 1, bt::Now());
            QVERIFY(!wnd.allPacketsAcked());
            QVERIFY(wnd.numUnackedPackets() == i + 1);
        }

        // Selectively ack the last 3 packets
        bt::Uint8 sack_data[6];
        memset(sack_data, 0, 6);
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        Ack(&sack, 8);
        Ack(&sack, 9);
        Ack(&sack, 10);
        Header hdr;
        hdr.ack_nr = 0;
        hdr.wnd_size = 5000;
        for (int i = 1; i < 8; i++) {
            retransmit_seq_nr.insert(i);
        }
        wnd.packetReceived(&hdr, &sack, this);
        QVERIFY(wnd.numUnackedPackets() == 7);
        QVERIFY(update_rtt_called);
        QVERIFY(retransmit_ok);
    }

    void testMultipleRetransmits3()
    {
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        for (bt::Uint32 i = 0; i < 10; i++) {
            wnd.addPacket(pkt, i + 1, bt::Now());
            QVERIFY(!wnd.allPacketsAcked());
            QVERIFY(wnd.numUnackedPackets() == i + 1);
        }

        // Selectively ack 3 random packets
        bt::Uint8 sack_data[6];
        memset(sack_data, 0, 6);
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        Ack(&sack, 3);
        Ack(&sack, 6);
        Ack(&sack, 10);
        Header hdr;
        hdr.ack_nr = 0;
        hdr.wnd_size = 5000;
        for (int i = 1; i <= 2; i++) {
            retransmit_seq_nr.insert(i);
        }
        wnd.packetReceived(&hdr, &sack, this);
        QVERIFY(wnd.numUnackedPackets() == 7);
        QVERIFY(update_rtt_called);
        QVERIFY(retransmit_ok);
    }

    void testWrapAround()
    {
        bt::Out(SYS_UTP | LOG_NOTICE) << "testWrapAround " << bt::endl;
        reset();
        PacketBuffer pkt;
        pkt.fillDummyData(200);

        RemoteWindow wnd;
        bt::Uint16 seq_nr = 65530;
        for (bt::Uint32 i = 0; i < 10; i++) {
            wnd.addPacket(pkt, seq_nr + i, bt::Now());
            QVERIFY(!wnd.allPacketsAcked());
            QVERIFY(wnd.numUnackedPackets() == i + 1);
        }

        // Selectively ack 3 random packets
        bt::Uint8 sack_data[6];
        memset(sack_data, 0, 6);
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        Ack(&sack, 3);
        Ack(&sack, 6);
        Ack(&sack, 10);
        Header hdr;
        hdr.ack_nr = seq_nr - 1;
        hdr.wnd_size = 5000;
        for (int i = 0; i < 2; i++) {
            retransmit_seq_nr.insert(seq_nr + i);
        }
        wnd.packetReceived(&hdr, &sack, this);
        QVERIFY(wnd.numUnackedPackets() == 7);
        QVERIFY(update_rtt_called);
        QVERIFY(retransmit_ok);
    }
};

QTEST_MAIN(RemoteWindowTest)

#include "remotewindowtest.moc"
