/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <array>

#include <QObject>
#include <QTest>

#include <net/poll.h>
#include <net/socket.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/pipe.h>

using namespace net;
using namespace bt;
using namespace Qt::Literals::StringLiterals;

class PollTest : public QEventLoop
{
    Q_OBJECT
public:
public Q_SLOTS:

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(bt::InitLibKTorrent());
        bt::InitLog(u"polltest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testInput()
    {
        Poll p;
        Pipe pipe;

        QCOMPARE_GE(pipe.readerSocket(), 0);
        QCOMPARE_GE(pipe.writerSocket(), 0);
        QCOMPARE(p.add(pipe.readerSocket(), Poll::INPUT), 0);
        char test[] = "TEST";
        QCOMPARE(pipe.write((const bt::Uint8 *)test, 4), 4);
        QCOMPARE(p.poll(), 1);
        QVERIFY(p.ready(0, net::Poll::INPUT));

        bt::Uint8 tmp[20];
        QCOMPARE(pipe.read(tmp, 20), 4);
        QCOMPARE(memcmp(tmp, test, 4), 0);
    }

    void testOutput()
    {
        Poll p;
        const Pipe pipe;

        QCOMPARE_GE(pipe.readerSocket(), 0);
        QCOMPARE_GE(pipe.writerSocket(), 0);
        QCOMPARE(p.add(pipe.writerSocket(), Poll::OUTPUT), 0);
        QCOMPARE(p.poll(), 1);
    }

    void testMultiplePolls()
    {
        Poll p;
        Pipe pipe;

        QCOMPARE_GE(pipe.readerSocket(), 0);
        QCOMPARE_GE(pipe.writerSocket(), 0);

        char test[] = "TEST";
        QCOMPARE(pipe.write((const bt::Uint8 *)test, 4), 4);

        for (int i = 0; i < 10; i++) {
            QCOMPARE(p.add(pipe.readerSocket(), Poll::INPUT), 0);
            QCOMPARE(p.poll(), 1);
            QVERIFY(p.ready(0, net::Poll::INPUT));
            p.reset();
        }

        bt::Uint8 tmp[20];
        QCOMPARE(pipe.read(tmp, 20), 4);
        QCOMPARE(memcmp(tmp, test, 4), 0);
        QCOMPARE(p.poll(100), 0);
    }

    void testTimeout()
    {
        Poll p;
        const Pipe pipe;

        QCOMPARE_GE(pipe.readerSocket(), 0);
        QCOMPARE_GE(pipe.writerSocket(), 0);
        QCOMPARE(p.add(pipe.readerSocket(), Poll::INPUT), 0);
        QCOMPARE(p.poll(100), 0);
    }

    void testSocket_data()
    {
        QTest::addColumn<int>("writer_ip_version");
        QTest::addColumn<int>("reader_ip_version");

        QTest::newRow("IPv4 -> IPv4") << 4 << 4;
        QTest::newRow("IPv6 -> IPv4") << 6 << 4;
        QTest::newRow("IPv6 -> IPv6") << 6 << 6;
    }

    void testSocket()
    {
        QFETCH(int, writer_ip_version);
        QFETCH(int, reader_ip_version);

        net::Socket sock(true, reader_ip_version);
        QVERIFY(sock.bind(reader_ip_version == 4 ? u"127.0.0.1"_s : u"::1"_s, 0, true));

        const net::Address local_addr = sock.getSockName();
        net::Socket writer(true, writer_ip_version);
        writer.setBlocking(false);
        writer.connectTo(local_addr);

        net::Address dummy;
        net::Poll poll;
        sock.prepare(&poll, net::Poll::INPUT);

        QCOMPARE_GT(poll.poll(1000), 0);
        const int fd = sock.accept(dummy);
        QCOMPARE_GE(fd, 0);

        poll.reset();
        QVERIFY(writer.connectSuccessful());

        net::Socket reader(fd, reader_ip_version);

        constexpr std::array<bt::Uint8, 20> data = {0xFF};
        QCOMPARE(writer.send(data), data.size());
        reader.prepare(&poll, net::Poll::INPUT);

        QCOMPARE_GT(poll.poll(1000), 0);

        bt::Uint8 tmp[20];
        QCOMPARE(reader.recv(tmp, 20), 20);
        QCOMPARE(memcmp(tmp, data.data(), 20), 0);
    }

private:
};

QTEST_MAIN(PollTest)

#include "polltest.moc"
