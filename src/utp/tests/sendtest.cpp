/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QObject>
#include <QTest>
#include <QTimer>

#include <torrent/globals.h>
#include <util/log.h>
#include <utp/utpserver.h>
#include <utp/utpsocket.h>

using namespace utp;
using namespace Qt::Literals::StringLiterals;

class SendTest : public QEventLoop
{
public:
    void accepted()
    {
        incoming = new UTPSocket(bt::Globals::instance().getUTPServer().acceptedConnection());
        exit();
    }

    void endEventLoop()
    {
        exit();
    }

private:
    void initTestCase()
    {
        bt::InitLog(u"sendtest.log"_s);

        incoming = nullptr;
        port = 50000;
        while (port < 60000) {
            if (!bt::Globals::instance().initUTPServer(port))
                port++;
            else
                break;
        }

        bt::Globals::instance().getUTPServer().setCreateSockets(false);
    }

    void cleanupTestCase()
    {
        delete incoming;
        delete outgoing;
        bt::Globals::instance().shutdownUTPServer();
    }

    void testConnect()
    {
        bt::Out(SYS_UTP | LOG_DEBUG) << "testConnect" << bt::endl;

        net::Address addr(u"127.0.0.1"_s, port);
        utp::UTPServer &srv = bt::Globals::instance().getUTPServer();
        connect(&srv, &utp::UTPServer::accepted, this, &SendTest::accepted, Qt::QueuedConnection);
        outgoing = new utp::UTPSocket();
        outgoing->setBlocking(false);
        outgoing->connectTo(addr);

        QTimer::singleShot(5000, this, &SendTest::endEventLoop); // use a 5 second timeout
        exec();
        QVERIFY(incoming != nullptr);

        // Wait until connection is complete
        int times = 0;
        while (!outgoing->connectSuccesFull() && times < 5) {
            QTest::qSleep(1000);
            times++;
        }
    }

    void testSend()
    {
        bt::Out(SYS_UTP | LOG_DEBUG) << "testSend" << bt::endl;
        outgoing->setBlocking(true);
        char test[] = "TEST";

        int ret = outgoing->send((const bt::Uint8 *)test, strlen(test));
        QVERIFY(ret == (int)strlen(test));

        char tmp[20];
        memset(tmp, 0, 20);
        ret = incoming->recv((bt::Uint8 *)tmp, 20);
        QVERIFY(ret == 4);
        QVERIFY(memcmp(tmp, test, ret) == 0);
    }

    void testSend2()
    {
        bt::Out(SYS_UTP | LOG_DEBUG) << "testSend2" << bt::endl;
        bt::Uint8 *sdata = new bt::Uint8[1000];
        outgoing->send(sdata, 1000);

        bt::Uint8 *rdata = new bt::Uint8[1000];
        int ret = incoming->recv(rdata, 1000);
        QVERIFY(ret == 1000);
        QVERIFY(memcmp(sdata, rdata, ret) == 0);

        delete[] rdata;
        delete[] sdata;
    }

    void testSend3()
    {
        bt::Out(SYS_UTP | LOG_DEBUG) << "testSend3" << bt::endl;
        char test[] = "TEST";

        outgoing->send((const bt::Uint8 *)test, strlen(test));
        incoming->send((const bt::Uint8 *)test, strlen(test));

        char tmp[20];
        memset(tmp, 0, 20);
        int ret = incoming->recv((bt::Uint8 *)tmp, 20);
        QVERIFY(ret == 4);
        QVERIFY(memcmp(tmp, test, ret) == 0);

        memset(tmp, 0, 20);
        ret = outgoing->recv((bt::Uint8 *)tmp, 20);
        QVERIFY(ret == 4);
        QVERIFY(memcmp(tmp, test, ret) == 0);
    }

    void testSend4()
    {
        bt::Out(SYS_UTP | LOG_DEBUG) << "testSend4" << bt::endl;
        char test[] = "TEST";

        outgoing->send((const bt::Uint8 *)test, strlen(test));
        outgoing->send((const bt::Uint8 *)test, strlen(test));

        char tmp[20];
        memset(tmp, 0, 20);
        int ret = incoming->recv((bt::Uint8 *)tmp, 20);
        QVERIFY(ret == 4 || ret == 8);
        QVERIFY(memcmp(tmp, test, 4) == 0);
        if (ret != 8) {
            memset(tmp, 0, 20);
            ret = incoming->recv((bt::Uint8 *)tmp, 20);
            QVERIFY(ret == 4);
            QVERIFY(memcmp(tmp, test, ret) == 0);
        } else {
            QVERIFY(memcmp(tmp + 4, test, 4) == 0);
        }
    }

private:
    int port;
    utp::UTPSocket *incoming;
    utp::UTPSocket *outgoing;
};

QTEST_MAIN(SendTest)
