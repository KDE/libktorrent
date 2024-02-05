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

class ConnectTest : public QEventLoop
{
public:
    void accepted()
    {
        accepted_conn = srv.acceptedConnection().toStrongRef();
        exit();
    }

    void endEventLoop()
    {
        exit();
    }

    void startConnect()
    {
        net::Address addr("127.0.0.1", port);
        srv.connectTo(addr);
        QTimer::singleShot(5000, this, &ConnectTest::endEventLoop); // use a 5 second timeout
    }

private:
    void initTestCase()
    {
        bt::InitLog("connecttest.log");

        port = 50000;
        while (port < 60000) {
            if (!srv.changePort(port))
                port++;
            else
                break;
        }

        srv.setCreateSockets(false);
        srv.start();
        QVERIFY(port < 60000);
    }

    void cleanupTestCase()
    {
        srv.stop();
    }

    void testConnect()
    {
        connect(&srv, &utp::UTPServer::accepted, this, &ConnectTest::accepted, Qt::QueuedConnection);
        QTimer::singleShot(0, this, &ConnectTest::startConnect);
        exec();
        QVERIFY(accepted_conn != nullptr);
    }

private:
    utp::UTPServer srv;
    int port;
    utp::Connection::Ptr accepted_conn;
};

QTEST_MAIN(ConnectTest)
