/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <interfaces/peerinterface.h>
#include <peer/packetreader.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

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

    void handlePacket(const bt::Uint8 *packet, bt::Uint32 size) override
    {
        received_packet.reset(new bt::Uint8[size]);
        memcpy(received_packet.data(), packet, size);
        received_packet_size = size;
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
        bt::InitLog(u"packetreadertest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testOnePacket()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(data, 14);
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

        pr.onDataReady(data, 14);
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data + 4, 10));

        pr.onDataReady(data2, 14);
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data2 + 4, 10));
    }

    void testChunked_data()
    {
        QTest::addColumn<bt::Uint32>("chunk_size");

        QTest::addRow("7") << 7u;
        QTest::addRow("1") << 1u;
    }

    void testChunked()
    {
        QFETCH(bt::Uint32, chunk_size);

        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        const auto num_full_chunks = std::size(data) / chunk_size;
        QCOMPARE_GT(num_full_chunks, 1); // Ensure data is actually chunked

        size_t num_bytes_read = 0;
        for (size_t i = 0; i < (num_full_chunks - 1); ++i) {
            pr.onDataReady(data + num_bytes_read, chunk_size);
            QVERIFY(pr.ok());
            pr.update(*this);
            QVERIFY(received_packet_size == 0);
            num_bytes_read += chunk_size;
        }

        const auto remaining_data_len = std::size(data) - num_bytes_read;
        pr.onDataReady(data + num_bytes_read, remaining_data_len);
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data + 4, 10));
    }

    void testIncompleteLength()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(data, 3);
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(received_packet_size == 0);

        pr.onDataReady(data + 3, 11);
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(check(data + 4, 10));
    }

    void testLengthToLarge()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(7);

        pr.onDataReady(data, 14);
        QVERIFY(!pr.ok());

        pr.update(*this);
        QVERIFY(received_packet_size == 0);

        // Subsequent attempts to write data should also fail
        pr.onDataReady(data, 1);
        QVERIFY(!pr.ok());

        pr.update(*this);
        QVERIFY(received_packet_size == 0);
    }

    void testPacketLengthZero()
    {
        reset();

        std::array<bt::Uint8, 10> data = {0, 0, 0, 0, 0, 0, 0, 2, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(data.data(), data.size());
        QVERIFY(pr.ok());

        pr.update(*this);
        QVERIFY(received_packet_size == 2);
    }

    void testUnicodeLiteral()
    {
        QString a = u"%1Torrent"_s.arg(QChar(0x00B5));
        QVERIFY(a == QStringLiteral("µTorrent"));
    }

private:
    QScopedArrayPointer<bt::Uint8> received_packet;
    bt::Uint32 received_packet_size;
};

QTEST_MAIN(PacketReaderTest)

#include "packetreadertest.moc"
