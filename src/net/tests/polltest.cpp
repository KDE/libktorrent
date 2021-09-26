/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QtTest>
#include <net/poll.h>
#include <net/socket.h>
#include <util/log.h>
#include <util/pipe.h>

using namespace net;
using namespace bt;

class PollTest : public QEventLoop
{
    Q_OBJECT
public:
public Q_SLOTS:

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("polltest.log");
    }

    void cleanupTestCase()
    {
    }

    void testInput()
    {
        Poll p;
        Pipe pipe;

        QVERIFY(pipe.readerSocket() >= 0);
        QVERIFY(pipe.writerSocket() >= 0);
        QVERIFY(p.add(pipe.readerSocket(), Poll::INPUT) == 0);
        char test[] = "TEST";
        QVERIFY(pipe.write((const bt::Uint8 *)test, 4) == 4);
        QVERIFY(p.poll() == 1);
        QVERIFY(p.ready(0, net::Poll::INPUT));

        bt::Uint8 tmp[20];
        QVERIFY(pipe.read(tmp, 20) == 4);
        QVERIFY(memcmp(tmp, test, 4) == 0);
    }

    void testOutput()
    {
        Poll p;
        Pipe pipe;

        QVERIFY(pipe.readerSocket() >= 0);
        QVERIFY(pipe.writerSocket() >= 0);
        QVERIFY(p.add(pipe.writerSocket(), Poll::OUTPUT) == 0);
        QVERIFY(p.poll() == 1);
    }

    void testMultiplePolls()
    {
        Poll p;
        Pipe pipe;

        QVERIFY(pipe.readerSocket() >= 0);
        QVERIFY(pipe.writerSocket() >= 0);

        char test[] = "TEST";
        QVERIFY(pipe.write((const bt::Uint8 *)test, 4) == 4);

        for (int i = 0; i < 10; i++) {
            QVERIFY(p.add(pipe.readerSocket(), Poll::INPUT) == 0);
            QVERIFY(p.poll() == 1);
            QVERIFY(p.ready(0, net::Poll::INPUT));
            p.reset();
        }

        bt::Uint8 tmp[20];
        QVERIFY(pipe.read(tmp, 20) == 4);
        QVERIFY(memcmp(tmp, test, 4) == 0);
        QVERIFY(p.poll(100) == 0);
    }

    void testTimeout()
    {
        Poll p;
        Pipe pipe;

        QVERIFY(pipe.readerSocket() >= 0);
        QVERIFY(pipe.writerSocket() >= 0);
        QVERIFY(p.add(pipe.readerSocket(), Poll::INPUT) == 0);
        QVERIFY(p.poll(100) == 0);
    }

    void testSocket()
    {
        net::Socket sock(true, 4);
        QVERIFY(sock.bind("127.0.0.1", 0, true));

        net::Address local_addr = sock.getSockName();
        net::Socket writer(true, 4);
        writer.setBlocking(false);
        writer.connectTo(local_addr);

        net::Address dummy;
        net::Poll poll;
        sock.prepare(&poll, net::Poll::INPUT);

        QVERIFY(poll.poll(1000) > 0);
        int fd = sock.accept(dummy);
        QVERIFY(fd >= 0);

        poll.reset();
        QVERIFY(writer.connectSuccesFull());

        net::Socket reader(fd, 6);

        bt::Uint8 data[20];
        memset(data, 0xFF, 20);
        QVERIFY(writer.send(data, 20) == 20);
        reader.prepare(&poll, net::Poll::INPUT);

        QVERIFY(poll.poll(1000) > 0);

        bt::Uint8 tmp[20];
        QVERIFY(reader.recv(tmp, 20) == 20);
        QVERIFY(memcmp(tmp, data, 20) == 0);
    }

private:
};

QTEST_MAIN(PollTest)

#include "polltest.moc"
