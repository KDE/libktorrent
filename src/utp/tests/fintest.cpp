/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>
#include <QTimer>

#include <util/log.h>
#include <utp/connection.h>
#include <utp/utpserver.h>

using namespace utp;
using namespace Qt::Literals::StringLiterals;

class FinTest : public QEventLoop
{
public:
    void accepted()
    {
        incoming = srv.acceptedConnection().toStrongRef();
        exit();
    }

    void endEventLoop()
    {
        exit();
    }

private:
    void initTestCase()
    {
        bt::InitLog(u"fintest.log"_s);

        port = 50000;
        while (port < 60000) {
            if (!srv.changePort(port))
                port++;
            else
                break;
        }

        srv.setCreateSockets(false);
        srv.start();
    }

    void cleanupTestCase()
    {
        srv.stop();
    }

    void testConnect()
    {
        net::Address addr(u"127.0.0.1"_s, port);
        connect(&srv, &utp::UTPServer::accepted, this, &FinTest::accepted, Qt::QueuedConnection);
        outgoing = srv.connectTo(addr).toStrongRef();
        QVERIFY(outgoing);
        QTimer::singleShot(5000, this, &FinTest::endEventLoop); // use a 5 second timeout
        exec();
        QVERIFY(incoming);
    }

    void testFin()
    {
        bt::Out(SYS_UTP | LOG_DEBUG) << "testFin" << bt::endl;
        if (outgoing->connectionState() != CS_CONNECTED || incoming->connectionState() != CS_CONNECTED) {
            QSKIP("Not Connected", SkipAll);
            return;
        }

        char test[] = "This is the fin test";
        outgoing->send((const bt::Uint8 *)test, strlen(test));
        incoming->setBlocking(true);
        if (incoming->waitForData()) {
            bt::Uint8 tmp[100];
            memset(tmp, 0, 100);
            int ret = incoming->recv(tmp, 100);
            QVERIFY(ret == (int)strlen(test));
            QVERIFY(memcmp(tmp, test, strlen(test)) == 0);

            outgoing->close();
            if (incoming->connectionState() != CS_CLOSED)
                incoming->waitForData();

            // connection should be closed now
            ret = incoming->recv(tmp, 100);
            QVERIFY(incoming->connectionState() == CS_CLOSED);
            QVERIFY(ret == -1);
        } else
            QFAIL("No data received");
    }

private:
    utp::UTPServer srv;
    int port;
    Connection::Ptr incoming;
    Connection::Ptr outgoing;
};

QTEST_MAIN(FinTest)
