/**
 * SPDX-FileCopyrightText: 2026 Jack Hill <jackhill3103@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <chrono>
#include <vector>

#include <QObject>
#include <QTest>

#include <diskio/chunkmanager.h>
#include <download/request.h>
#include <net/packetsocket.h>
#include <torrent/torrent.h>
#include <util/constants.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

#include <dummytorrentcreator.h>
#include <utils.h>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

struct PiecePacket {
    bt::PeerMessageType m_packet_type = bt::PeerMessageType::PIECE;
    bt::Uint32 m_chunk_index;
    bt::Uint32 m_offset;
    bt::Uint32 m_length;
    bt::Chunk *m_chunk;

    bt::Uint32 size() const
    {
        return 4 + 1 + 4 + 4 + m_length;
    }

    bt::Packet::Ptr toPacket() const
    {
        return bt::Packet::create(m_chunk_index, m_offset, m_length, m_chunk);
    }

    bool verifyBuffer(QByteArrayView buffer) const
    {
        if (buffer.size() < (size())) {
            bt::Out() << "Piece::VerifyBuffer incorrect buffer size, expected " << size() << " got " << buffer.size() << bt::endl;
            return false;
        }

        const auto bytes = reinterpret_cast<const bt::Uint8 *>(buffer.data());

        // size does not include the message length at the start of the buffer
        const auto packet_size = bt::ReadUint32(bytes, 0);
        if (bt::ReadUint32(bytes, 0) != (size() - 4)) {
            bt::Out() << "Piece::VerifyBuffer incorrect message size, expected " << size() << " got " << packet_size << bt::endl;
            return false;
        }

        const auto packet_type = bytes[4];
        if (packet_type != m_packet_type) {
            bt::Out() << "Piece::VerifyBuffer incorrect packet type, expected " << m_packet_type << " got " << packet_type << bt::endl;
            return false;
        }

        const auto chunk_index = bt::ReadUint32(bytes, 5);
        if (chunk_index != m_chunk_index) {
            bt::Out() << "Piece::VerifyBuffer incorrect chunk index, expected " << m_chunk_index << " got " << chunk_index << bt::endl;
            return false;
        }

        const auto offset = bt::ReadUint32(bytes, 9);
        if (offset != m_offset) {
            bt::Out() << "Piece::VerifyBuffer incorrect piece offset, expected " << m_offset << " got " << offset << bt::endl;
            return false;
        }

        const auto piece_data = buffer.sliced(13, m_length);
        if (piece_data != QByteArrayView{m_chunk->getPiece(m_offset, m_length, true)->data(), m_length}) {
            bt::Out() << "Piece::VerifyBuffer incorrect piece data" << bt::endl;
            return false;
        }

        return true;
    }
};

struct ExtensionPacket {
    bt::PeerMessageType m_packet_type = bt::PeerMessageType::EXTENDED;
    bt::Uint8 m_extension_id;
    QByteArray m_message;

    bt::Uint32 size() const
    {
        return 4 + 1 + 1 + static_cast<bt::Uint32>(m_message.size());
    }

    bt::Packet::Ptr toPacket() const
    {
        return bt::Packet::create(m_extension_id, m_message);
    }

    bool verifyBuffer(QByteArrayView buffer) const
    {
        if (buffer.size() < size()) {
            bt::Out() << "Piece::VerifyBuffer incorrect buffer size, expected " << size() << " got " << buffer.size() << bt::endl;
            return false;
        }

        const auto bytes = reinterpret_cast<const bt::Uint8 *>(buffer.data());

        // size does not include the message length at the start of the buffer
        const auto packet_size = bt::ReadUint32(bytes, 0);
        if (bt::ReadUint32(bytes, 0) != (size() - 4)) {
            bt::Out() << "Piece::VerifyBuffer incorrect message size, expected " << size() << " got " << packet_size << bt::endl;
            return false;
        }

        const auto packet_type = bytes[4];
        if (packet_type != m_packet_type) {
            bt::Out() << "Piece::VerifyBuffer incorrect packet type, expected " << m_packet_type << " got " << packet_type << bt::endl;
            return false;
        }

        const auto extension_id = bytes[5];
        if (extension_id != m_extension_id) {
            bt::Out() << "Piece::VerifyBuffer incorrect extension ID, expected " << m_extension_id << " got " << extension_id << bt::endl;
            return false;
        }

        const auto message = buffer.sliced(6, m_message.size());
        if (message != m_message) {
            bt::Out() << "Piece::VerifyBuffer incorrect message" << bt::endl;
            return false;
        }

        return true;
    }
};

struct RequestPacket {
    bt::PeerMessageType m_packet_type = bt::PeerMessageType::REQUEST;
    bt::Uint32 m_chunk_index;
    bt::Uint32 m_offset;
    bt::Uint32 m_length;

    bt::Uint32 size() const
    {
        return 4 + 1 + 4 + 4 + 4;
    }

    bt::Packet::Ptr toPacket() const
    {
        return bt::Packet::create(bt::Request{m_chunk_index, m_offset, m_length, nullptr}, m_packet_type);
    }

    bool verifyBuffer(QByteArrayView buffer) const
    {
        if (buffer.size() < (size())) {
            bt::Out() << "Piece::VerifyBuffer incorrect buffer size, expected " << size() << " got " << buffer.size() << bt::endl;
            return false;
        }

        const auto bytes = reinterpret_cast<const bt::Uint8 *>(buffer.data());

        // size does not include the message length at the start of the buffer
        const auto packet_size = bt::ReadUint32(bytes, 0);
        if (bt::ReadUint32(bytes, 0) != (size() - 4)) {
            bt::Out() << "Piece::VerifyBuffer incorrect message size, expected " << size() << " got " << packet_size << bt::endl;
            return false;
        }

        const auto packet_type = bytes[4];
        if (packet_type != m_packet_type) {
            bt::Out() << "Piece::VerifyBuffer incorrect packet type, expected " << m_packet_type << " got " << packet_type << bt::endl;
            return false;
        }

        const auto chunk_index = bt::ReadUint32(bytes, 5);
        if (chunk_index != m_chunk_index) {
            bt::Out() << "Piece::VerifyBuffer incorrect chunk index, expected " << m_chunk_index << " got " << chunk_index << bt::endl;
            return false;
        }

        const auto offset = bt::ReadUint32(bytes, 9);
        if (offset != m_offset) {
            bt::Out() << "Piece::VerifyBuffer incorrect piece offset, expected " << m_offset << " got " << offset << bt::endl;
            return false;
        }

        const auto length = bt::ReadUint32(bytes, 13);
        if (length != m_length) {
            bt::Out() << "Piece::VerifyBuffer incorrect piece offset, expected " << m_length << " got " << length << bt::endl;
            return false;
        }

        return true;
    }
};

class PacketSocketTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(bt::InitLibKTorrent());
        bt::InitLog(u"packetsockettest.log"_s, false, true, true);

        constexpr bt::Uint64 TEST_FILE_SIZE = 15 * 1024 * 1024;
        QMap<QString, bt::Uint64> files;
        files[u"aaa.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"bbb.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);
        files[u"ccc.avi"_s] = RandomSize(TEST_FILE_SIZE / 2, TEST_FILE_SIZE);

        QVERIFY(m_creator.createMultiFileTorrent(files, u"movies"_s));

        bt::Out(SYS_GEN | LOG_DEBUG) << "Created " << m_creator.torrentPath() << bt::endl;
        try {
            m_tor.load(bt::LoadFile(m_creator.torrentPath()), false);
        } catch (bt::Error &err) {
            bt::Out(SYS_GEN | LOG_DEBUG) << "Failed to load torrent: " << m_creator.torrentPath() << bt::endl;
            QFAIL("Torrent load failure");
        }
    }

    void testSendNothing()
    {
        auto socket_pair = CreateSocketPair();
        QVERIFY(socket_pair.has_value());
        net::PacketSocket packet_socket(std::move(socket_pair->writer));

        QCOMPARE(packet_socket.write(0, bt::Now()), 0);
    }

    void testChunkedWrite()
    {
        auto socket_pair = CreateSocketPair();
        QVERIFY(socket_pair.has_value());
        net::PacketSocket packet_socket(std::move(socket_pair->writer));

        const ExtensionPacket test_extension_packet{
            .m_extension_id = 0,
            .m_message = QByteArrayLiteral("{'m': {'ut_metadata', 3}, 'metadata_size': 31235}"),
        };

        packet_socket.addPacket(test_extension_packet.toPacket());

        QVERIFY(packet_socket.bytesReadyToWrite());
        QCOMPARE(packet_socket.numPendingPieceUploads(), 0);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 0);

        constexpr bt::Uint32 chunk_size = 1;
        bt::Uint32 bytes_uploaded = 0;
        while (packet_socket.bytesReadyToWrite()) {
            bytes_uploaded += packet_socket.write(chunk_size, bt::Now());
        }

        QCOMPARE(bytes_uploaded, test_extension_packet.size());

        QVERIFY(!packet_socket.bytesReadyToWrite());
        // We sent a control packet, so no PIECE packets should have been uploaded.
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);

        // Wait for reader socket to get data
        QThread::sleep(100ms);

        QCOMPARE(socket_pair->reader->bytesAvailable(), test_extension_packet.size());
        std::vector<bt::Uint8> receive_packet(test_extension_packet.size());
        socket_pair->reader->recv(receive_packet.data(), receive_packet.size());

        QVERIFY(test_extension_packet.verifyBuffer(receive_packet));
    }

    void test3ControlTo1DataPacketRatio()
    {
        auto socket_pair = CreateSocketPair();
        QVERIFY(socket_pair.has_value());
        net::PacketSocket packet_socket(std::move(socket_pair->writer));

        bt::ChunkManager cman(m_tor, m_creator.tempPath(), m_creator.dataPath(), true, nullptr);
        constexpr bt::Uint32 chunk_index = 0;
        bt::Chunk *chunk = cman.getChunk(chunk_index);
        constexpr bt::Uint32 num_chunks = 10;

        // Queue up the piece packets first to prove that the order of being pushed into the socket doesn't matter
        const PiecePacket test_piece_packet{
            .m_chunk_index = chunk_index,
            .m_offset = 0,
            .m_length = static_cast<bt::Uint32>(m_tor.getChunkSize()) / num_chunks,
            .m_chunk = chunk,
        };

        for (size_t i = 0; i < 10; ++i) {
            packet_socket.addPacket(test_piece_packet.toPacket());
        }

        const ExtensionPacket test_extension_packet{
            .m_extension_id = 0,
            .m_message = QByteArrayLiteral("{'m': {'ut_metadata', 3}, 'metadata_size': 31235}"),
        };

        for (size_t i = 0; i < 10; ++i) {
            packet_socket.addPacket(test_extension_packet.toPacket());
        }

        std::vector<bt::Uint8> read_buffer(std::max(test_extension_packet.size(), test_piece_packet.size()));
        // Expect 3 control, 1 data, 3 control, 1 data, 3 control, 1 data
        for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j) {
                QCOMPARE(packet_socket.write(test_extension_packet.size(), bt::Now()), test_extension_packet.size());
                QCOMPARE(packet_socket.dataBytesUploaded(), 0);

                std::fill(read_buffer.begin(), read_buffer.end(), 0);
                QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_extension_packet.size());
                QVERIFY(test_extension_packet.verifyBuffer(read_buffer));
            }

            QCOMPARE(packet_socket.numPendingPieceUploads(), 10 - i);
            QCOMPARE(packet_socket.numPendingPieceUploadBytes(), (10 - i) * test_piece_packet.size());
            QCOMPARE(packet_socket.write(test_piece_packet.size(), bt::Now()), test_piece_packet.size());
            QCOMPARE(packet_socket.dataBytesUploaded(), test_piece_packet.size());

            std::fill(read_buffer.begin(), read_buffer.end(), 0);
            QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_piece_packet.size());
            QVERIFY(test_piece_packet.verifyBuffer(read_buffer));
        }

        QCOMPARE(packet_socket.numPendingPieceUploads(), 7);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 7 * test_piece_packet.size());

        // 1 more control packet left, then it only has data packets to send
        QCOMPARE(packet_socket.write(test_extension_packet.size(), bt::Now()), test_extension_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);

        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_extension_packet.size());
        QVERIFY(test_extension_packet.verifyBuffer(read_buffer));

        QCOMPARE(packet_socket.write(test_piece_packet.size(), bt::Now()), test_piece_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), test_piece_packet.size());

        QCOMPARE(packet_socket.numPendingPieceUploads(), 6);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 6 * test_piece_packet.size());

        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_piece_packet.size());
        QVERIFY(test_piece_packet.verifyBuffer(read_buffer));

        // Check that the next control packet is prioritised
        {
            packet_socket.addPacket(test_extension_packet.toPacket());
        }

        // However the socket will have already loaded in the next piece packet after finishing the previous packet. Flush it out first and then we should see
        // the next control packet.
        QCOMPARE(packet_socket.write(test_piece_packet.size(), bt::Now()), test_piece_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), test_piece_packet.size());

        QCOMPARE(packet_socket.numPendingPieceUploads(), 5);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 5 * test_piece_packet.size());

        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_piece_packet.size());
        QVERIFY(test_piece_packet.verifyBuffer(read_buffer));

        QCOMPARE(packet_socket.write(test_extension_packet.size(), bt::Now()), test_extension_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);

        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_extension_packet.size());
        QVERIFY(test_extension_packet.verifyBuffer(read_buffer));

        // Remaining piece packets should flush out
        for (bt::Uint32 i = 0; i < 5; ++i) {
            QCOMPARE(packet_socket.write(test_piece_packet.size(), bt::Now()), test_piece_packet.size());
            QCOMPARE(packet_socket.dataBytesUploaded(), test_piece_packet.size());
            QCOMPARE(packet_socket.numPendingPieceUploads(), 4 - i);
            QCOMPARE(packet_socket.numPendingPieceUploadBytes(), (4 - i) * test_piece_packet.size());

            std::fill(read_buffer.begin(), read_buffer.end(), 0);
            QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_piece_packet.size());
            QVERIFY(test_piece_packet.verifyBuffer(read_buffer));
        }

        QCOMPARE(packet_socket.numPendingPieceUploads(), 0);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 0);
        QVERIFY(!packet_socket.bytesReadyToWrite());

        // Now check that all control packets are sent if there are no data packets
        for (size_t i = 0; i < 4; ++i) {
            packet_socket.addPacket(test_extension_packet.toPacket());
        }
        QVERIFY(packet_socket.bytesReadyToWrite());
        for (bt::Uint32 i = 0; i < 4; ++i) {
            QCOMPARE(packet_socket.write(test_extension_packet.size(), bt::Now()), test_extension_packet.size());
            QCOMPARE(packet_socket.dataBytesUploaded(), 0);

            std::fill(read_buffer.begin(), read_buffer.end(), 0);
            QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_extension_packet.size());
            QVERIFY(test_extension_packet.verifyBuffer(read_buffer));
        }
        QVERIFY(!packet_socket.bytesReadyToWrite());
    }

    void testClearPieces()
    {
        auto socket_pair = CreateSocketPair();
        QVERIFY(socket_pair.has_value());
        net::PacketSocket packet_socket(std::move(socket_pair->writer));

        bt::ChunkManager cman(m_tor, m_creator.tempPath(), m_creator.dataPath(), true, nullptr);
        constexpr bt::Uint32 chunk_index = 0;
        bt::Chunk *chunk = cman.getChunk(chunk_index);
        constexpr bt::Uint32 num_chunks = 10;
        const auto piece_length = static_cast<bt::Uint32>(m_tor.getChunkSize()) / num_chunks;

        const PiecePacket test_piece_packet1{
            .m_chunk_index = chunk_index,
            .m_offset = 0,
            .m_length = piece_length,
            .m_chunk = chunk,
        };

        const PiecePacket test_piece_packet2{
            .m_chunk_index = chunk_index,
            .m_offset = piece_length,
            .m_length = piece_length,
            .m_chunk = chunk,
        };

        packet_socket.addPacket(test_piece_packet1.toPacket());
        packet_socket.addPacket(test_piece_packet2.toPacket());
        packet_socket.addPacket(test_piece_packet2.toPacket());
        packet_socket.addPacket(test_piece_packet1.toPacket());

        std::vector<bt::Uint8> read_buffer(test_piece_packet1.size());
        QCOMPARE(packet_socket.write(5, bt::Now()), 5);

        QCOMPARE(packet_socket.numPendingPieceUploads(), 4);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 4 * test_piece_packet1.size() - 5);

        constexpr bool send_reject = true;
        packet_socket.clearPieces(send_reject);
        QCOMPARE(packet_socket.numPendingPieceUploads(), 1);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), test_piece_packet1.size() - 5);

        QCOMPARE(packet_socket.write(test_piece_packet1.size() - 5, bt::Now()), test_piece_packet1.size() - 5);
        QCOMPARE(packet_socket.numPendingPieceUploads(), 0);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 0);
        QCOMPARE(packet_socket.dataBytesUploaded(), test_piece_packet1.size());

        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_piece_packet1.size());
        QVERIFY(test_piece_packet1.verifyBuffer(read_buffer));

        const RequestPacket test_reject_piece1_packet{
            .m_packet_type = bt::PeerMessageType::REJECT_REQUEST,
            .m_chunk_index = test_piece_packet1.m_chunk_index,
            .m_offset = test_piece_packet1.m_offset,
            .m_length = test_piece_packet1.m_length,
        };

        const RequestPacket test_reject_piece2_packet{
            .m_packet_type = bt::PeerMessageType::REJECT_REQUEST,
            .m_chunk_index = test_piece_packet2.m_chunk_index,
            .m_offset = test_piece_packet2.m_offset,
            .m_length = test_piece_packet2.m_length,
        };

        QVERIFY(packet_socket.bytesReadyToWrite());

        QCOMPARE(packet_socket.write(test_reject_piece2_packet.size(), bt::Now()), test_reject_piece2_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);
        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_reject_piece2_packet.size());
        QVERIFY(test_reject_piece2_packet.verifyBuffer(read_buffer));

        // Check the second reject is sent too
        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QVERIFY(packet_socket.bytesReadyToWrite());

        QCOMPARE(packet_socket.write(test_reject_piece2_packet.size(), bt::Now()), test_reject_piece2_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);
        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_reject_piece2_packet.size());
        QVERIFY(test_reject_piece2_packet.verifyBuffer(read_buffer));
        QVERIFY(packet_socket.bytesReadyToWrite());

        // Check the third reject is sent too
        std::fill(read_buffer.begin(), read_buffer.end(), 0);

        QCOMPARE(packet_socket.write(test_reject_piece1_packet.size(), bt::Now()), test_reject_piece1_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);
        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_reject_piece1_packet.size());
        QVERIFY(test_reject_piece1_packet.verifyBuffer(read_buffer));
        QVERIFY(!packet_socket.bytesReadyToWrite());
    }

    void testDoNotSendPiece()
    {
        auto socket_pair = CreateSocketPair();
        QVERIFY(socket_pair.has_value());
        net::PacketSocket packet_socket(std::move(socket_pair->writer));

        bt::ChunkManager cman(m_tor, m_creator.tempPath(), m_creator.dataPath(), true, nullptr);
        constexpr bt::Uint32 chunk_index = 0;
        bt::Chunk *chunk = cman.getChunk(chunk_index);
        constexpr bt::Uint32 num_chunks = 10;
        const auto piece_length = static_cast<bt::Uint32>(m_tor.getChunkSize()) / num_chunks;

        const PiecePacket test_piece_packet1{
            .m_chunk_index = chunk_index,
            .m_offset = 0,
            .m_length = piece_length,
            .m_chunk = chunk,
        };

        const PiecePacket test_piece_packet2{
            .m_chunk_index = chunk_index,
            .m_offset = piece_length,
            .m_length = piece_length,
            .m_chunk = chunk,
        };

        packet_socket.addPacket(test_piece_packet1.toPacket());
        packet_socket.addPacket(test_piece_packet2.toPacket());
        packet_socket.addPacket(test_piece_packet2.toPacket());
        packet_socket.addPacket(test_piece_packet1.toPacket());

        std::vector<bt::Uint8> read_buffer(test_piece_packet1.size());
        QCOMPARE(packet_socket.write(5, bt::Now()), 5);

        QCOMPARE(packet_socket.numPendingPieceUploads(), 4);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 4 * test_piece_packet1.size() - 5);

        constexpr bool send_reject = true;
        packet_socket.doNotSendPiece(
            bt::Request{
                test_piece_packet2.m_chunk_index,
                test_piece_packet2.m_offset,
                test_piece_packet2.m_length,
                nullptr,
            },
            send_reject);
        QCOMPARE(packet_socket.numPendingPieceUploads(), 2);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 2 * test_piece_packet1.size() - 5);

        constexpr bool do_not_send_reject = false;
        packet_socket.doNotSendPiece(
            bt::Request{
                test_piece_packet1.m_chunk_index,
                test_piece_packet1.m_offset,
                test_piece_packet1.m_length,
                nullptr,
            },
            do_not_send_reject);
        QCOMPARE(packet_socket.numPendingPieceUploads(), 1);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), test_piece_packet1.size() - 5);

        QCOMPARE(packet_socket.write(test_piece_packet1.size() - 5, bt::Now()), test_piece_packet1.size() - 5);
        QCOMPARE(packet_socket.numPendingPieceUploads(), 0);
        QCOMPARE(packet_socket.numPendingPieceUploadBytes(), 0);
        QCOMPARE(packet_socket.dataBytesUploaded(), test_piece_packet1.size());

        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_piece_packet1.size());
        QVERIFY(test_piece_packet1.verifyBuffer(read_buffer));

        const RequestPacket test_reject_piece2_packet{
            .m_packet_type = bt::PeerMessageType::REJECT_REQUEST,
            .m_chunk_index = test_piece_packet2.m_chunk_index,
            .m_offset = test_piece_packet2.m_offset,
            .m_length = test_piece_packet2.m_length,
        };

        QVERIFY(packet_socket.bytesReadyToWrite());

        QCOMPARE(packet_socket.write(test_reject_piece2_packet.size(), bt::Now()), test_reject_piece2_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);
        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_reject_piece2_packet.size());
        QVERIFY(test_reject_piece2_packet.verifyBuffer(read_buffer));

        // Check the second reject is sent too
        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        QVERIFY(packet_socket.bytesReadyToWrite());

        QCOMPARE(packet_socket.write(test_reject_piece2_packet.size(), bt::Now()), test_reject_piece2_packet.size());
        QCOMPARE(packet_socket.dataBytesUploaded(), 0);
        QCOMPARE(socket_pair->reader->recv(read_buffer.data(), read_buffer.size()), test_reject_piece2_packet.size());
        QVERIFY(test_reject_piece2_packet.verifyBuffer(read_buffer));
        QVERIFY(!packet_socket.bytesReadyToWrite());
    }

private:
    DummyTorrentCreator m_creator;
    bt::Torrent m_tor;
};

QTEST_MAIN(PacketSocketTest)

#include "packetsockettest.moc"
