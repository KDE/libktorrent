/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <interfaces/peerinterface.h>
#include <peer/packetreader.h>
#include <util/log.h>

class PacketReaderTest : public QObject, public bt::PeerInterface
{
    Q_OBJECT
public:
    PacketReaderTest(QObject *parent = nullptr)
        : QObject(parent)
        , bt::PeerInterface(bt::PeerID(), 100)
    {
    }

    void chunkAllowed(bt::Uint32 chunk) override
    {
        Q_UNUSED(chunk);
    }

    void handlePacket(QByteArrayView packet) override
    {
        received_packet.reset(new bt::Uint8[packet.size()]);
        memcpy(received_packet.data(), packet.constData(), packet.size());
        received_packet_size = packet.size();
    }

    bool check(const bt::Uint8 *packet, bt::Uint32 size)
    {
        return received_packet_size == size && memcmp(packet, received_packet.data(), size) == 0;
    }

    bt::Uint32 averageDownloadSpeed() const override
    {
        return 0;
    }

    void kill() override
    {
    }

    void reset()
    {
        received_packet_size = 0;
        received_packet.reset();
    }

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("packetreadertest.log");
    }

    void cleanupTestCase()
    {
    }

    void testOnePacket()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(QByteArrayView(data, 14));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data + 4, 10));
    }

    void testMultiplePackets()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::Uint8 data2[] = {0, 0, 0, 10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        bt::PacketReader pr(1024);

        pr.onDataReady(QByteArrayView(data, 14));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data + 4, 10));

        pr.onDataReady(QByteArrayView(data2, 14));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data2 + 4, 10));
    }

    void testChunked()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(QByteArrayView(data, 7));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(received_packet_size == 0);

        pr.onDataReady(QByteArrayView(data + 7, 7));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data + 4, 10));
    }

    void testIncompleteLength()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(QByteArrayView(data, 3));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(received_packet_size == 0);

        pr.onDataReady(QByteArrayView(data + 3, 11));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data + 4, 10));
    }

    void testLengthToLarge()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(7);

        pr.onDataReady(QByteArrayView(data, 14));
        QVERIFY(!pr.ok());

        pr.update(*this);
        QVERIFY(received_packet_size == 0);
    }

    void testUnicodeLiteral()
    {
        QString a = QString("%1Torrent").arg(QChar(0x00B5));
        QVERIFY(a == QStringLiteral("µTorrent"));
    }

private:
    QScopedArrayPointer<bt::Uint8> received_packet;
    bt::Uint32 received_packet_size;
};

QTEST_MAIN(PacketReaderTest)

#include "packetreadertest.moc"
