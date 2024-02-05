/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <peer/connectionlimit.h>
#include <util/log.h>

class ConnectionLimitTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("connectionlimittest.log");
    }

    void cleanupTestCase()
    {
    }

    void testGlobalLimit()
    {
        bt::ConnectionLimit climit;
        climit.setLimits(10, 20);
        bt::SHA1Hash hash;

        QList<bt::ConnectionLimit::Token::Ptr> tokens;
        for (int i = 0; i < 10; i++) {
            bt::ConnectionLimit::Token::Ptr t = climit.acquire(hash);
            QVERIFY(!t.isNull());
            tokens.append(t);
        }

        bt::ConnectionLimit::Token::Ptr t = climit.acquire(hash);
        QVERIFY(t.isNull());

        tokens.pop_front();

        t = climit.acquire(hash);
        QVERIFY(!t.isNull());
    }

    void testTorrentLimit()
    {
        bt::ConnectionLimit climit;
        climit.setLimits(20, 10);
        bt::SHA1Hash hash;

        QList<bt::ConnectionLimit::Token::Ptr> tokens;
        for (int i = 0; i < 10; i++) {
            bt::ConnectionLimit::Token::Ptr t = climit.acquire(hash);
            QVERIFY(!t.isNull());
            tokens.append(t);
        }

        bt::ConnectionLimit::Token::Ptr t = climit.acquire(hash);
        QVERIFY(t.isNull());

        tokens.pop_front();

        t = climit.acquire(hash);
        QVERIFY(!t.isNull());
    }

    void testMulitpleTorrents()
    {
        bt::Uint8 tmp[20];
        bt::ConnectionLimit climit;
        climit.setLimits(15, 10);

        memset(tmp, 0xFF, 20);
        bt::SHA1Hash hash1 = bt::SHA1Hash::generate(tmp, 20);

        memset(tmp, 0xEE, 20);
        bt::SHA1Hash hash2 = bt::SHA1Hash::generate(tmp, 20);

        QList<bt::ConnectionLimit::Token::Ptr> tokens;
        for (int i = 0; i < 10; i++) {
            bt::ConnectionLimit::Token::Ptr t = climit.acquire(hash1);
            QVERIFY(!t.isNull());
            tokens.append(t);
        }

        QVERIFY(climit.acquire(hash1).isNull());

        for (int i = 0; i < 5; i++) {
            bt::ConnectionLimit::Token::Ptr t = climit.acquire(hash2);
            QVERIFY(!t.isNull());
            tokens.append(t);
        }

        bt::ConnectionLimit::Token::Ptr t = climit.acquire(hash1);
        QVERIFY(t.isNull());

        t = climit.acquire(hash2);
        QVERIFY(t.isNull());

        tokens.pop_front();

        t = climit.acquire(hash2);
        QVERIFY(!t.isNull());
    }

private:
};

QTEST_MAIN(ConnectionLimitTest)

#include "connectionlimittest.moc"
