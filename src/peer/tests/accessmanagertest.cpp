/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTest>

#include <interfaces/blocklistinterface.h>
#include <peer/accessmanager.h>
#include <torrent/server.h>
#include <tracker/tracker.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

class TestBlockList : public bt::BlockListInterface
{
public:
    TestBlockList()
    {
    }

    ~TestBlockList() override
    {
    }

    [[nodiscard]] bool blocked(const net::Address &addr) const override
    {
        return addr.toString() == u"8.8.8.8"_s;
    }
};

class AccessManagerTest : public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"accessmanagertest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testCustomIP()
    {
        bt::Tracker::setCustomIP(u"123.123.123.123"_s);
        bt::Server::setPort(7777);
        QVERIFY(!bt::AccessManager::instance().allowed(net::Address(u"123.123.123.123"_s, 7777)));
        QVERIFY(bt::AccessManager::instance().allowed(net::Address(u"123.123.123.123"_s, 7776)));
    }

    void testExternalAddress()
    {
        bt::Server::setPort(7777);
        bt::AccessManager::instance().addExternalIP(u"12.12.12.12"_s);
        QVERIFY(!bt::AccessManager::instance().allowed(net::Address(u"12.12.12.12"_s, 7777)));
        QVERIFY(bt::AccessManager::instance().allowed(net::Address(u"12.12.12.12"_s, 7776)));
    }

    void testBlockList()
    {
        bt::AccessManager::instance().addBlockList(new TestBlockList());
        QVERIFY(!bt::AccessManager::instance().allowed(net::Address(u"8.8.8.8"_s, 7777)));
        QVERIFY(bt::AccessManager::instance().allowed(net::Address(u"8.8.8.9"_s, 7776)));
    }

private:
};

QTEST_MAIN(AccessManagerTest)

#include "accessmanagertest.moc"
