/*
 *  SPDX-FileCopyrightText: 2026 Jack Hill <jackhill3103@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <array>
#include <cstring>
#include <vector>

#include <QObject>
#include <QRandomGenerator>
#include <QTest>

#include <diskio/chunk.h>
#include <diskio/chunkmanager.h>
#include <download/packet.h>
#include <download/request.h>
#include <net/address.h>
#include <net/poll.h>
#include <net/socket.h>
#include <util/bitset.h>
#include <util/constants.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

#include <testlib/dummytorrentcreator.h>
#include <testlib/utils.h>

using namespace Qt::StringLiterals;

bool ComparePacketSize(const bt::Packet::Ptr &packet, const bt::Uint32 size)
{
    return packet->getDataLength() == (4 + size) && bt::ReadUint32(packet->getData(), 0), size;
}

bool ComparePacketType(const bt::Packet::Ptr &packet, const bt::PeerMessageType type)
{
    return packet->getData()[4] == type;
}

class PacketTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(bt::InitLibKTorrent());
        bt::InitLog(u"packettest.log"_s, false, true, true);
    }

    void testNoPayload_data()
    {
        QTest::addColumn<bt::PeerMessageType>("packet_type");

        QTest::addRow("CHOKE") << bt::PeerMessageType::CHOKE;
        QTest::addRow("UNCHOKE") << bt::PeerMessageType::UNCHOKE;
        QTest::addRow("INTERESTED") << bt::PeerMessageType::INTERESTED;
        QTest::addRow("NOT_INTERESTED") << bt::PeerMessageType::NOT_INTERESTED;
    }

    void testNoPayload()
    {
        QFETCH(bt::PeerMessageType, packet_type);
        const auto packet = bt::Packet::create(packet_type);
        QVERIFY(ComparePacketSize(packet, 1));
        QVERIFY(ComparePacketType(packet, packet_type));
    }

    void testPort()
    {
        constexpr bt::Uint16 port = 1337;
        const auto packet = bt::Packet::create(port);
        QVERIFY(ComparePacketSize(packet, 3));
        QVERIFY(ComparePacketType(packet, bt::PeerMessageType::PORT));
        QCOMPARE(bt::ReadUint16(packet->getData(), 5), port);
    }

    void testPieceIndex_data()
    {
        QTest::addColumn<bt::Uint32>("piece_index");
        QTest::addColumn<bt::PeerMessageType>("packet_type");

        QTest::addRow("HAVE") << 512u << bt::PeerMessageType::HAVE;
        QTest::addRow("SUGGEST_PIECE") << 1234u << bt::PeerMessageType::SUGGEST_PIECE;
        QTest::addRow("ALLOWED_FAST") << 98433215u << bt::PeerMessageType::ALLOWED_FAST;
    }

    void testPieceIndex()
    {
        QFETCH(bt::Uint32, piece_index);
        QFETCH(bt::PeerMessageType, packet_type);

        const auto packet = bt::Packet::create(piece_index, packet_type);
        QVERIFY(ComparePacketSize(packet, 5));
        QVERIFY(ComparePacketType(packet, packet_type));
        QCOMPARE(bt::ReadUint32(packet->getData(), 5), piece_index);
    }

    void testBitSet()
    {
        std::array<bt::Uint32, 100> bitset_data;
        constexpr bt::Uint32 num_bits = bitset_data.size() * sizeof(bitset_data[0]) * 8;
        QRandomGenerator::global()->generate(bitset_data.begin(), bitset_data.end());
        const bt::BitSet bitset{reinterpret_cast<bt::Uint8 *>(bitset_data.data()), num_bits};

        const auto packet = bt::Packet::create(bitset);
        QVERIFY(ComparePacketSize(packet, 1 + bitset.getNumBytes()));
        QVERIFY(ComparePacketType(packet, bt::PeerMessageType::BITFIELD));
        QCOMPARE(std::memcmp(packet->getData() + 5, bitset.getData(), bitset.getNumBytes()), 0);
    }

    void testRequest_data()
    {
        QTest::addColumn<bt::Request>("request");
        QTest::addColumn<bt::PeerMessageType>("packet_type");

        QTest::addRow("REQUEST") << bt::Request{5, 5 * bt::MAX_PIECE_LEN, bt::MAX_PIECE_LEN, nullptr} << bt::PeerMessageType::REQUEST;
        QTest::addRow("CANCEL") << bt::Request{999, 888, 777, nullptr} << bt::PeerMessageType::CANCEL;
        QTest::addRow("REJECT_REQUEST") << bt::Request{3, 300, 100, nullptr} << bt::PeerMessageType::REJECT_REQUEST;
    }

    void testRequest()
    {
        QFETCH(bt::Request, request);
        QFETCH(bt::PeerMessageType, packet_type);

        const auto packet = bt::Packet::create(request, packet_type);
        QVERIFY(ComparePacketSize(packet, 13));
        QVERIFY(ComparePacketType(packet, packet_type));
        QCOMPARE(bt::ReadUint32(packet->getData(), 5), request.getIndex());
        QCOMPARE(bt::ReadUint32(packet->getData(), 9), request.getOffset());
        QCOMPARE(bt::ReadUint32(packet->getData(), 13), request.getLength());
    }

    void testPiece()
    {
        constexpr bt::Uint64 TEST_FILE_SIZE = 15 * 1024 * 1024;
        QMap<QString, bt::Uint64> files;
        files[u"aaa.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"bbb.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"ccc.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        DummyTorrentCreator creator;
        bt::Torrent tor;
        QVERIFY(creator.createMultiFileTorrent(files, u"movies"_s));

        bt::Out(SYS_GEN | LOG_DEBUG) << "Created " << creator.torrentPath() << bt::endl;
        try {
            tor.load(bt::LoadFile(creator.torrentPath()), false);
        } catch (bt::Error &err) {
            bt::Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << creator.torrentPath() << bt::endl;
            QFAIL("Torrent load failure");
        }

        bt::ChunkManager cman(tor, creator.tempPath(), creator.dataPath(), true, nullptr);
        constexpr bt::Uint32 chunk_index = 0;
        bt::Chunk *chunk = cman.getChunk(chunk_index);

        constexpr bt::Uint32 num_pieces_to_send = 5;
        const auto piece_size = static_cast<bt::Uint32>(tor.getChunkSize()) / num_pieces_to_send;
        for (bt::Uint32 i = 0; i < num_pieces_to_send; ++i) {
            const auto offset = i * piece_size;
            const auto packet = bt::Packet::create(chunk_index, offset, piece_size, chunk);
            // message len (4) + message type (1) + chunk index (4) + offset into chunk (4) + piece size
            QVERIFY(ComparePacketSize(packet, 13 + piece_size));
            QVERIFY(ComparePacketType(packet, bt::PeerMessageType::PIECE));
            QCOMPARE(bt::ReadUint32(packet->getData(), 5), chunk_index);
            QCOMPARE(bt::ReadUint32(packet->getData(), 9), offset);

            constexpr bool read_only = true;
            auto piece = chunk->getPiece(offset, piece_size, read_only);
            QCOMPARE(memcmp(packet->getData() + 13, piece->data(), piece_size), 0);
        }
    }

    void testExtensionMessage()
    {
        constexpr uint8_t extension_id = 0;
        const auto message = QByteArrayLiteral("{'m': {'ut_metadata', 3}, 'metadata_size': 31235}");

        const auto packet = bt::Packet::create(extension_id, message);
        QVERIFY(ComparePacketSize(packet, 1 + message.size()));
        QVERIFY(ComparePacketType(packet, bt::PeerMessageType::EXTENDED));
        QCOMPARE(packet->getData()[5], extension_id);
        QCOMPARE(memcmp(packet->getData() + 6, message.data(), message.size()), 0);
    }

    void testMakeRejectOfPieceBad()
    {
        for (uint8_t type = 0; type < 255; ++type) {
            if (type == bt::PeerMessageType::PIECE) {
                continue;
            }
            auto packet = bt::Packet::create(type);
            QCOMPARE(packet->makeRejectOfPiece(), nullptr);
        }
    }

    void testSend()
    {
        constexpr uint8_t extension_id = 0;
        const auto message = QByteArrayLiteral("{'m': {'ut_metadata', 3}, 'metadata_size': 31235}");
        constexpr bt::Uint32 max_bytes_to_send = 10;

        // In case the message is changed, ensure that the data is chunked
        QCOMPARE_LT(max_bytes_to_send, message.size());

        auto packet = bt::Packet::create(extension_id, message);

        auto socket_pair = CreateSocketPair();
        QVERIFY(socket_pair.has_value());

        auto &writer_socket = socket_pair->writer;
        auto &reader_socket = socket_pair->reader;

        // Expected size is message len (4) + msg type (1) + extension id (1) + message size = 6 + message size
        const auto total_data_size = 6 + message.size();
        std::vector<bt::Uint8> reader_data(total_data_size, 0);
        auto expected_num_sends = total_data_size / max_bytes_to_send;
        if (total_data_size % max_bytes_to_send != 0) {
            ++expected_num_sends;
        }

        auto num_sends = 0;
        auto num_bytes_sent = 0;
        while (!packet->isSent() && num_sends < expected_num_sends) {
            QCOMPARE_LE(packet->send(writer_socket.get(), max_bytes_to_send), max_bytes_to_send);

            const auto bytes_received = reader_socket->recv(reinterpret_cast<bt::Uint8 *>(reader_data.data()) + num_bytes_sent, max_bytes_to_send);

            QCOMPARE_LE(bytes_received, max_bytes_to_send);
            num_bytes_sent += bytes_received;
            QCOMPARE_LE(num_bytes_sent, total_data_size);
            ++num_sends;
            bt::Out(SYS_GEN | LOG_DEBUG) << "Sent " << num_bytes_sent << " / " << total_data_size << " bytes " << num_sends << " / " << expected_num_sends
                                         << " sends" << bt::endl;
        }

        QCOMPARE(num_sends, expected_num_sends);
        QCOMPARE(bt::ReadUint32(reader_data.data(), 0), 2 + message.size());
        QCOMPARE(reader_data[4], bt::PeerMessageType::EXTENDED);
        QCOMPARE(reader_data[5], extension_id);
        QCOMPARE(QByteArrayView{reader_data}.sliced(6), message);
    }
};

QTEST_MAIN(PacketTest)

#include "packettest.moc"
