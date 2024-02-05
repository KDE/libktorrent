/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <util/log.h>
#include <utp/localwindow.h>
#include <utp/utpprotocol.h>

using namespace utp;
using namespace bt;

class LocalWindowTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("localwindowtest.log");
    }

    void testSeqNr()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testSeqNr" << endl;
        QVERIFY(SeqNrCmpSE(4, 8));
        QVERIFY(SeqNrCmpSE(4, 4));
        QVERIFY(!SeqNrCmpSE(4, 2));

        QVERIFY(SeqNrCmpS(4, 8));
        QVERIFY(!SeqNrCmpS(4, 4));
        QVERIFY(!SeqNrCmpS(4, 2));

        QVERIFY(!SeqNrCmpSE(1, 65000));
        QVERIFY(SeqNrCmpSE(65000, 1));
        QVERIFY(!SeqNrCmpS(1, 65000));
        QVERIFY(SeqNrCmpS(65000, 1));

        QVERIFY(SeqNrDiff(65535, 0) == 1);
        QVERIFY(SeqNrDiff(0, 65535) == 1);
        QVERIFY(SeqNrDiff(65535, 6) == 7);
        QVERIFY(SeqNrDiff(6, 65535) == 7);
        QVERIFY(SeqNrDiff(65530, 5) == 11);
        QVERIFY(SeqNrDiff(5, 65530) == 11);
    }

    void testLocalWindow()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testLocalWindow" << endl;
        bt::BufferPool pool;
        bt::Uint8 wdata[1000];
        memset(wdata, 0, 1000);

        utp::Header hdr;
        utp::LocalWindow wnd(1000);
        wnd.setLastSeqNr(1);

        // write 500 bytes to it
        hdr.seq_nr = 2;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(500), 0) == true);
        QVERIFY(wnd.availableSpace() == 500);
        QVERIFY(wnd.currentWindow() == 500);

        // write another 100 to it
        hdr.seq_nr++;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(100), 0) == true);
        QVERIFY(wnd.availableSpace() == 400);
        QVERIFY(wnd.currentWindow() == 600);

        // read 300 from it
        QVERIFY(wnd.read(wdata, 300) == 300);
        QVERIFY(wnd.availableSpace() == 700);
        QVERIFY(wnd.currentWindow() == 300);
    }

    void testPacketLoss()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testPacketLoss" << endl;
        bt::BufferPool pool;
        bt::Uint8 wdata[1000];

        utp::Header hdr;
        utp::LocalWindow wnd(1000);
        wnd.setLastSeqNr(1);

        // write 500 bytes to it
        hdr.seq_nr = 2;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(500), 0) == true);
        QVERIFY(wnd.availableSpace() == 500);
        QVERIFY(wnd.currentWindow() == 500);

        // write 100 bytes to it bit with the wrong sequence number
        hdr.seq_nr = 4;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(100), 0) == true);
        QVERIFY(wnd.availableSpace() == 400);
        QVERIFY(wnd.currentWindow() == 600);
        QVERIFY(wnd.bytesAvailable() == 500);

        // Try to read all of it, but we should only get back 500
        QVERIFY(wnd.read(wdata, 600) == 500);
        QVERIFY(wnd.availableSpace() == 900);
        QVERIFY(wnd.currentWindow() == 100);
        QVERIFY(wnd.bytesAvailable() == 0);

        // write the missing packet
        hdr.seq_nr = 3;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(100), 0) == true);
        QVERIFY(wnd.availableSpace() == 800);
        QVERIFY(wnd.currentWindow() == 200);
        QVERIFY(wnd.bytesAvailable() == 200);
    }

    void testPacketLoss2()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testPacketLoss2" << endl;
        bt::Uint8 wdata[1000];
        memset(wdata, 0, 1000);

        bt::BufferPool pool;

        utp::Header hdr;
        utp::LocalWindow wnd(1000);
        wnd.setLastSeqNr(0);

        // first write first and last packet
        bt::Uint32 step = 200;
        hdr.seq_nr = 1;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - step);
        QVERIFY(wnd.currentWindow() == step);
        hdr.seq_nr = 5;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 2 * step);
        QVERIFY(wnd.currentWindow() == 2 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // Now write 4
        hdr.seq_nr = 4;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 3 * step);
        QVERIFY(wnd.currentWindow() == 3 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // And then 3
        hdr.seq_nr = 3;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 4 * step);
        QVERIFY(wnd.currentWindow() == 4 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // And then 2
        hdr.seq_nr = 2;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 5 * step);
        QVERIFY(wnd.currentWindow() == 5 * step);
        QVERIFY(wnd.bytesAvailable() == 5 * step);
    }

    void testToMuchData()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testToMuchData" << endl;
        bt::BufferPool pool;
        bt::Uint8 wdata[1000];
        memset(wdata, 0, 1000);

        utp::Header hdr;
        utp::LocalWindow wnd(500);
        wnd.setLastSeqNr(1);

        // write 500 bytes to it
        hdr.seq_nr = 2;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(500), 0) == true);
        QVERIFY(wnd.availableSpace() == 0);
        QVERIFY(wnd.currentWindow() == 500);

        // writing more data should now have no effect at all
        hdr.seq_nr = 3;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(500), 0) == false);
        QVERIFY(wnd.availableSpace() == 0);
        QVERIFY(wnd.currentWindow() == 500);
    }

    void testSelectiveAck()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testSelectiveAck" << endl;
        bt::BufferPool pool;
        bt::Uint8 wdata[1000];
        memset(wdata, 0, 1000);

        utp::Header hdr;
        utp::LocalWindow wnd(1000);
        wnd.setLastSeqNr(0);

        // first write first and last packet
        bt::Uint32 step = 200;
        hdr.seq_nr = 1;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - step);
        QVERIFY(wnd.currentWindow() == step);
        hdr.seq_nr = 5;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 2 * step);
        QVERIFY(wnd.currentWindow() == 2 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // Check SelectiveAck generation
        QVERIFY(wnd.selectiveAckBits() == 3);
        bt::Uint8 sack_data[6];
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        wnd.fillSelectiveAck(&sack);
        QVERIFY(sack_data[2] == 0x4);
        QVERIFY(sack_data[3] == 0x0);
        QVERIFY(sack_data[4] == 0x0);
        QVERIFY(sack_data[5] == 0x0);

        // Now write 4
        hdr.seq_nr = 4;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 3 * step);
        QVERIFY(wnd.currentWindow() == 3 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // Check selective ack again
        QVERIFY(wnd.selectiveAckBits() == 3);
        sack.length = 4;
        sack.extension = 0;
        wnd.fillSelectiveAck(&sack);
        QVERIFY(sack_data[2] == 0x6);
        QVERIFY(sack_data[3] == 0x0);
        QVERIFY(sack_data[4] == 0x0);
        QVERIFY(sack_data[5] == 0x0);

        // Now write 3
        hdr.seq_nr = 3;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 4 * step);
        QVERIFY(wnd.currentWindow() == 4 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // Check selective ack again
        QVERIFY(wnd.selectiveAckBits() == 3);
        sack.length = 4;
        sack.extension = 0;
        wnd.fillSelectiveAck(&sack);
        QVERIFY(sack_data[2] == 0x7);
        QVERIFY(sack_data[3] == 0x0);
        QVERIFY(sack_data[4] == 0x0);
        QVERIFY(sack_data[5] == 0x0);

        // And then 2
        hdr.seq_nr = 2;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 5 * step);
        QVERIFY(wnd.currentWindow() == 5 * step);
        QVERIFY(wnd.bytesAvailable() == 5 * step);
        // selective ack should now be unnecessary
        QVERIFY(wnd.selectiveAckBits() == 0);
    }

    void testSeqNrWrapAround()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testSeqNrWrapAround" << endl;
        bt::BufferPool pool;
        bt::Uint8 wdata[1000];
        memset(wdata, 0, 1000);

        utp::Header hdr;
        utp::LocalWindow wnd(1000);
        wnd.setLastSeqNr(65535);

        // write 500 bytes to it
        hdr.seq_nr = 0;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(500), 0) == true);
        QVERIFY(wnd.availableSpace() == 500);
        QVERIFY(wnd.currentWindow() == 500);

        // write another 100 to it
        hdr.seq_nr++;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(100), 0) == true);
        QVERIFY(wnd.availableSpace() == 400);
        QVERIFY(wnd.currentWindow() == 600);

        // read 300 from it
        QVERIFY(wnd.read(wdata, 300) == 300);
        QVERIFY(wnd.availableSpace() == 700);
        QVERIFY(wnd.currentWindow() == 300);
    }

    void testSeqNrWrapAroundSelectiveAck()
    {
        Out(SYS_UTP | LOG_DEBUG) << "testSeqNrWrapAroundSelectiveAck" << endl;
        bt::BufferPool pool;
        bt::Uint8 wdata[1000];
        memset(wdata, 0, 1000);

        utp::Header hdr;
        utp::LocalWindow wnd(1000);
        wnd.setLastSeqNr(65535);

        // first write first and last packet
        bt::Uint32 step = 200;
        hdr.seq_nr = 0;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - step);
        QVERIFY(wnd.currentWindow() == step);
        hdr.seq_nr = 4;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 2 * step);
        QVERIFY(wnd.currentWindow() == 2 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // Check SelectiveAck generation
        QVERIFY(wnd.selectiveAckBits() == 3);
        bt::Uint8 sack_data[6];
        SelectiveAck sack;
        sack.length = 4;
        sack.extension = 0;
        sack.bitmask = sack_data + 2;
        wnd.fillSelectiveAck(&sack);
        QVERIFY(sack_data[2] == 0x4);
        QVERIFY(sack_data[3] == 0x0);
        QVERIFY(sack_data[4] == 0x0);
        QVERIFY(sack_data[5] == 0x0);

        // Now write 4
        hdr.seq_nr = 3;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 3 * step);
        QVERIFY(wnd.currentWindow() == 3 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // Check selective ack again
        QVERIFY(wnd.selectiveAckBits() == 3);
        sack.length = 4;
        sack.extension = 0;
        wnd.fillSelectiveAck(&sack);
        QVERIFY(sack_data[2] == 0x6);
        QVERIFY(sack_data[3] == 0x0);
        QVERIFY(sack_data[4] == 0x0);
        QVERIFY(sack_data[5] == 0x0);

        // Now write 3
        hdr.seq_nr = 2;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 4 * step);
        QVERIFY(wnd.currentWindow() == 4 * step);
        QVERIFY(wnd.bytesAvailable() == step);

        // Check selective ack again
        QVERIFY(wnd.selectiveAckBits() == 3);
        sack.length = 4;
        sack.extension = 0;
        wnd.fillSelectiveAck(&sack);
        QVERIFY(sack_data[2] == 0x7);
        QVERIFY(sack_data[3] == 0x0);
        QVERIFY(sack_data[4] == 0x0);
        QVERIFY(sack_data[5] == 0x0);

        // And then 2
        hdr.seq_nr = 1;
        QVERIFY(wnd.packetReceived(&hdr, pool.get(step), 0) == true);
        QVERIFY(wnd.availableSpace() == wnd.windowCapacity() - 5 * step);
        QVERIFY(wnd.currentWindow() == 5 * step);
        QVERIFY(wnd.bytesAvailable() == 5 * step);
        // selective ack should now be unnecessary
        QVERIFY(wnd.selectiveAckBits() == 0);
    }

private:
    // bt::Uint8 data[13];
    // bt::Uint8 data2[6];
};

QTEST_MAIN(LocalWindowTest)

#include "localwindowtest.moc"
