/***************************************************************************
 *   Copyright (C) 2012 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include <QtTest>
#include <QObject>
#include <util/log.h>
#include <peer/packetreader.h>
#include <interfaces/peerinterface.h>


class PacketReaderTest : public QObject, public bt::PeerInterface
{
    Q_OBJECT
public:
    PacketReaderTest(QObject* parent = 0) : QObject(parent), bt::PeerInterface(bt::PeerID(), 100)
    {}

    void chunkAllowed(bt::Uint32 chunk) override
    {
        Q_UNUSED(chunk);
    }

    void handlePacket(const bt::Uint8* packet, bt::Uint32 size) override
    {
        received_packet.reset(new bt::Uint8[size]);
        memcpy(received_packet.data(), packet, size);
        received_packet_size = size;
    }

    bool check(const bt::Uint8* packet, bt::Uint32 size)
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

    void testChunked()
    {
        reset();

        bt::Uint8 data[] = {0, 0, 0, 10, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE};
        bt::PacketReader pr(1024);

        pr.onDataReady(data, 7);
        QVERIFY(pr.ok());
        pr.update(*this);
        QVERIFY(received_packet_size == 0);

        pr.onDataReady(data + 7, 7);
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

