/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <array>
#include <cstddef>

#include <QObject>
#include <QTest>

#include <peer/connectionlimit.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

class ConnectionLimitTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"connectionlimittest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testGlobalLimit()
    {
        bt::ConnectionLimit climit;
        climit.setLimits(10, 20);
        const bt::SHA1Hash hash;

        std::vector<std::unique_ptr<bt::ConnectionLimit::Token>> tokens;
        for (int i = 0; i < 10; i++) {
            std::unique_ptr<bt::ConnectionLimit::Token> t = climit.acquire(hash);
            QVERIFY(t);
            tokens.push_back(std::move(t));
        }

        std::unique_ptr<bt::ConnectionLimit::Token> t = climit.acquire(hash);
        QVERIFY(!t);

        tokens.erase(tokens.cbegin());

        t = climit.acquire(hash);
        QVERIFY(t);
    }

    void testTorrentLimit()
    {
        bt::ConnectionLimit climit;
        climit.setLimits(20, 10);
        const bt::SHA1Hash hash;

        std::vector<std::unique_ptr<bt::ConnectionLimit::Token>> tokens;
        for (int i = 0; i < 10; i++) {
            std::unique_ptr<bt::ConnectionLimit::Token> t = climit.acquire(hash);
            QVERIFY(t);
            tokens.push_back(std::move(t));
        }

        std::unique_ptr<bt::ConnectionLimit::Token> t = climit.acquire(hash);
        QVERIFY(!t);

        tokens.erase(tokens.cbegin());

        t = climit.acquire(hash);
        QVERIFY(t);
    }

    void testMulitpleTorrents()
    {
        std::array<std::byte, 20> tmp;
        bt::ConnectionLimit climit;
        climit.setLimits(15, 10);

        tmp.fill(std::byte{0xFF});
        const bt::SHA1Hash hash1 = bt::SHA1Hash::generate(tmp);

        tmp.fill(std::byte{0xEE});
        const bt::SHA1Hash hash2 = bt::SHA1Hash::generate(tmp);

        std::vector<std::unique_ptr<bt::ConnectionLimit::Token>> tokens;
        for (int i = 0; i < 10; i++) {
            std::unique_ptr<bt::ConnectionLimit::Token> t = climit.acquire(hash1);
            QVERIFY(t);
            tokens.push_back(std::move(t));
        }

        QVERIFY(!climit.acquire(hash1));

        for (int i = 0; i < 5; i++) {
            std::unique_ptr<bt::ConnectionLimit::Token> t = climit.acquire(hash2);
            QVERIFY(t);
            tokens.push_back(std::move(t));
        }

        std::unique_ptr<bt::ConnectionLimit::Token> t = climit.acquire(hash1);
        QVERIFY(!t);

        t = climit.acquire(hash2);
        QVERIFY(!t);

        tokens.erase(tokens.cbegin());

        t = climit.acquire(hash2);
        QVERIFY(t);
    }

private:
};

QTEST_MAIN(ConnectionLimitTest)

#include "connectionlimittest.moc"
