/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <array>

#include <QByteArrayView>
#include <QObject>
#include <QTest>

#include <interfaces/peerinterface.h>
#include <peer/packetreader.h>
#include <util/constants.h>
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

    void handlePacket(QByteArrayView packet) override
    {
        received_packet = packet.toByteArray();
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
        received_packet.clear();
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

        constexpr std::array<bt::Uint8, 14> data = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(data);
        QVERIFY(pr.ok());
        pr.update(*this);
        QCOMPARE(received_packet, QByteArrayView{data}.sliced(4));
    }

    void testMultiplePackets()
    {
        reset();

        constexpr std::array<bt::Uint8, 14> data = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        constexpr std::array<bt::Uint8, 14> data2 = {0, 0, 0, 10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        bt::PacketReader pr(1024);

        pr.onDataReady(data);
        QVERIFY(pr.ok());
        pr.update(*this);
        QCOMPARE(received_packet, QByteArrayView{data}.sliced(4));

        pr.onDataReady(data2);
        QVERIFY(pr.ok());
        pr.update(*this);
        QCOMPARE(received_packet, QByteArrayView{data2}.sliced(4));
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

        constexpr std::array<bt::Uint8, 14> data = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        QByteArrayView data_view{data};
        bt::PacketReader pr(1024);

        const auto num_full_chunks = std::size(data) / chunk_size;
        QCOMPARE_GT(num_full_chunks, 1); // Ensure data is actually chunked

        for (size_t i = 0; i < (num_full_chunks - 1); ++i) {
            pr.onDataReady(data_view.first(chunk_size));
            data_view.slice(chunk_size);
            QVERIFY(pr.ok());
            pr.update(*this);
            QVERIFY(received_packet.isEmpty());
        }

        pr.onDataReady(data_view);
        QVERIFY(pr.ok());
        pr.update(*this);
        QCOMPARE(received_packet, QByteArrayView{data}.sliced(4));
    }

    void testIncompleteLength()
    {
        reset();

        constexpr std::array<bt::Uint8, 14> data = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        QByteArrayView data_view{data};
        bt::PacketReader pr(1024);

        pr.onDataReady(data_view.first(3));
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(received_packet.isEmpty());

        pr.onDataReady(data_view.sliced(3));
        QVERIFY(pr.ok());
        pr.update(*this);
        QCOMPARE(received_packet, QByteArrayView{data}.sliced(4));
    }

    void testLengthToLarge()
    {
        reset();

        constexpr std::array<bt::Uint8, 14> data = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(7);

        pr.onDataReady(data);
        QVERIFY(!pr.ok());

        pr.update(*this);
        QVERIFY(received_packet.isEmpty());

        // Subsequent attempts to write data should also fail
        pr.onDataReady(QByteArrayView{data}.first(1));
        QVERIFY(!pr.ok());

        pr.update(*this);
        QVERIFY(received_packet.isEmpty());
    }

    void testPacketLengthZero()
    {
        reset();

        constexpr std::array<bt::Uint8, 10> data = {0, 0, 0, 0, 0, 0, 0, 2, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(data);
        QVERIFY(pr.ok());

        pr.update(*this);
        QCOMPARE(received_packet.size(), 2);
    }

    void testUnicodeLiteral()
    {
        const QString a = u"%1Torrent"_s.arg(QChar(0x00B5));
        QCOMPARE(a, QStringLiteral("µTorrent"));
    }

private:
    QByteArray received_packet;
};

QTEST_MAIN(PacketReaderTest)

#include "packetreadertest.moc"
