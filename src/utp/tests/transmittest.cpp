/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QFile>
#include <QObject>
#include <QTest>
#include <QTextStream>
#include <QTimer>

#include <util/functions.h>
#include <util/log.h>
#include <util/sha1hash.h>
#include <util/sha1hashgen.h>
#include <utp/connection.h>
#include <utp/utpserver.h>
#include <utp/utpsocket.h>

#define BYTES_TO_SEND 100 * 1024 * 1024

using namespace utp;
using namespace bt;
using namespace Qt::Literals::StringLiterals;

static QByteArray Generate(int size)
{
    QByteArray ba(size, 0);
    /*  for (int i = 0;i < size;i+=4)
        {
            ba[i] = 'A';
            ba[i+1] = 'B';
            ba[i+2] = 'C';
            ba[i+3] = 'D';
        }
        */
    for (int i = 0; i < size; i++) {
        ba[i] = i % 256;
    }
    return ba;
}
/*
static void Dump(const bt::Uint8* pkt, bt::Uint32 size,const QString & file)
{
    QFile fptr(file);
    if (fptr.open(QIODevice::Text|QIODevice::WriteOnly))
    {
        QTextStream out(&fptr);
        out << "Packet: " << size << ::endl;
        out << "Hash:   " << bt::SHA1Hash::generate(pkt,size).toString() << ::endl;

        for (bt::Uint32 i = 0;i < size;i+=4)
        {
            if (i > 0 && i % 32 == 0)
                out << ::endl;

            out << QString("%1%2%3%4 ")
                    .arg(pkt[i],2,16)
                    .arg(pkt[i+1],2,16)
                    .arg(pkt[i+2],2,16)
                    .arg(pkt[i+3],2,16);
        }

        out << ::endl << ::endl << ::endl;
    }
}
*/

class SendThread : public QThread
{
public:
    SendThread(Connection::Ptr outgoing, UTPServer &srv, QObject *parent = nullptr)
        : QThread(parent)
        , outgoing(outgoing)
        , srv(srv)
    {
    }

    void run() override
    {
        int step = 64 * 1024;
        const QByteArray data = Generate(step);
        const QByteArrayView data_view{data};
        bt::SHA1HashGen hgen;

        bt::Int64 sent = 0;
        int off = 0;
        net::Poll poller;
        while (sent < BYTES_TO_SEND && outgoing->connectionState() != CS_CLOSED) {
            int to_send = step - off;
            int ret = outgoing->send((const bt::Uint8 *)data.data() + off, to_send);
            if (ret > 0) {
                hgen.update(data_view.sliced(off, ret));
                sent += ret;
                off += ret;
                off = off % step;
                // Out(SYS_UTP|LOG_DEBUG) << "Transmitted " << sent << endl;
            } else if (ret == 0) {
                srv.preparePolling(&poller, net::Poll::OUTPUT, outgoing);
                poller.poll(1000);
            } else {
                break;
            }
        }

        sleep(2);
        Out(SYS_UTP | LOG_DEBUG) << "Transmitted " << sent << endl;
        outgoing->dumpStats();
        outgoing->close();
        hgen.end();
        sent_hash = hgen.get();
    }

    Connection::Ptr outgoing;
    bt::SHA1Hash sent_hash;
    UTPServer &srv;
};

class TransmitTest : public QEventLoop
{
    Q_OBJECT
public:
    TransmitTest(QObject *parent = nullptr)
        : QEventLoop(parent)
    {
    }

    void accepted()
    {
        incoming = srv.acceptedConnection().toStrongRef();
        incoming->setBlocking(true);
        exit();
    }

    void startConnect()
    {
        net::Address addr(u"127.0.0.1"_s, port);
        outgoing = srv.connectTo(addr);
        outgoing->setBlocking(true);
    }

    void endEventLoop()
    {
        exit();
    }

private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLibKTorrent();
        bt::InitLog(u"transmittest.log"_s, false, true, false);

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
        connect(&srv, &utp::UTPServer::accepted, this, &TransmitTest::accepted, Qt::QueuedConnection);
        QTimer::singleShot(0, this, &TransmitTest::startConnect);
        QTimer::singleShot(5000, this, &TransmitTest::endEventLoop); // use a 5 second timeout
        exec();
        QVERIFY(outgoing);
        QVERIFY(incoming);
        QVERIFY(incoming->connectionState() == CS_CONNECTED);
        if (outgoing->connectionState() != CS_CONNECTED)
            QVERIFY(outgoing->waitUntilConnected());
        QVERIFY(outgoing->connectionState() == CS_CONNECTED);
    }

    void testThreaded()
    {
        bt::Out(SYS_UTP | LOG_DEBUG) << "testThreaded" << bt::endl;
        if (outgoing->connectionState() != CS_CONNECTED || incoming->connectionState() != CS_CONNECTED) {
            QSKIP("Not Connected", SkipAll);
            return;
        }

        bt::SHA1HashGen hgen;

        SendThread st(outgoing, srv);
        st.start(); // The thread will start sending a whole bunch of data
        bt::Int64 received = 0;
        // int failures = 0;
        incoming->setBlocking(true);
        while (received < BYTES_TO_SEND && incoming->connectionState() != CS_CLOSED) {
            bt::Uint32 ba = incoming->bytesAvailable();
            if (ba > 0) {
                // failures = 0;
                QByteArray data(ba, 0);
                int to_read = ba; //;qMin<bt::Uint32>(1024,ba);
                int ret = incoming->recv((bt::Uint8 *)data.data(), to_read);
                QVERIFY(ret == to_read);
                if (ret > 0) {
                    hgen.update(data);
                    received += ret;
                    // Out(SYS_UTP|LOG_DEBUG) << "Received " << received << endl;
                }
            } else if (incoming->connectionState() != CS_CLOSED) {
                incoming->waitForData(1000);
            }
        }

        st.wait();
        Out(SYS_UTP | LOG_DEBUG) << "Received " << received << endl;
        incoming->dumpStats();
        QVERIFY(incoming->bytesAvailable() == 0);
        QVERIFY(outgoing->allDataSent());
        QVERIFY(received >= BYTES_TO_SEND);

        hgen.end();
        SHA1Hash rhash = hgen.get();
        Out(SYS_UTP | LOG_DEBUG) << "Received data hash: " << rhash.toString() << endl;
        Out(SYS_UTP | LOG_DEBUG) << "Sent data hash:     " << st.sent_hash.toString() << endl;
        QVERIFY(rhash == st.sent_hash);
    }

private:
    Connection::Ptr incoming;
    Connection::Ptr outgoing;
    utp::UTPServer srv;
    int port;
};

QTEST_MAIN(TransmitTest)

#include "transmittest.moc"
