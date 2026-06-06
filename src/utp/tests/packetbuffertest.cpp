/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include <util/circularbuffer.h>
#include <util/log.h>
#include <utp/packetbuffer.h>
#include <utp/utpprotocol.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

class PacketBufferTest : public QObject
{
    Q_OBJECT
public:
    void initTestCase()
    {
        bt::InitLog(u"packetbuffertest.log"_s);
    }

    void cleanupTestCase()
    {
    }

private Q_SLOTS:
    void testPacketBuffer()
    {
        constexpr std::array<bt::Uint8, 200> tmp{0xFF};

        bt::CircularBuffer cbuf;
        cbuf.write(tmp);

        utp::PacketBuffer pbuf;
        QCOMPARE(pbuf.fillData(cbuf, 200), 200);
        QCOMPARE(pbuf.payloadSize(), 200);
        QCOMPARE(pbuf.bufferSize(), 200);
        QCOMPARE(memcmp(pbuf.data(), tmp.data(), 200), 0);

        utp::Header hdr;
        memset(&hdr, 0, sizeof(utp::Header));
        hdr.ack_nr = 600;
        hdr.connection_id = 1777;
        hdr.extension = 0;
        hdr.seq_nr = 1000;
        hdr.timestamp_difference_microseconds = 0;
        hdr.timestamp_microseconds = 0;
        hdr.version = 0;
        hdr.wnd_size = 6000;

        pbuf.setHeader(hdr, 0);
        QCOMPARE(pbuf.bufferSize(), 200 + utp::Header::size());
        QCOMPARE(memcmp(pbuf.data() + utp::Header::size(), tmp.data(), 200), 0);
        QCOMPARE(pbuf.payloadSize(), 200);

        utp::Header hdr2;
        memset(&hdr2, 0, sizeof(utp::Header));
        hdr2.read(pbuf.data());
        QCOMPARE(memcmp(&hdr, &hdr2, sizeof(utp::Header)), 0);

        pbuf.setHeader(hdr, 4);
        QCOMPARE(pbuf.bufferSize(), 200 + utp::Header::size() + 4);
        QCOMPARE(memcmp(pbuf.data() + utp::Header::size() + 4, tmp.data(), 200), 0);
        QCOMPARE(pbuf.payloadSize(), 200);

        utp::Header hdr3;
        memset(&hdr3, 0, sizeof(utp::Header));
        hdr3.read(pbuf.data());
        QCOMPARE(memcmp(&hdr, &hdr3, sizeof(utp::Header)), 0);
    }

    void testData()
    {
        bt::Uint8 tmp[200];
        memset(tmp, 0xFF, 200);

        utp::PacketBuffer pbuf;
        pbuf.fillData(tmp, 200);
        QCOMPARE(pbuf.bufferSize(), 200);
        QCOMPARE(pbuf.payloadSize(), 200);
        QCOMPARE(memcmp(pbuf.data(), tmp, 200), 0);
    }
};

QTEST_MAIN(PacketBufferTest)

#include "packetbuffertest.moc"
