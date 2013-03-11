/***************************************************************************
 *   Copyright (C) 2012 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include <QtTest>
#include <QObject>
#include <util/log.h>
#include <peer/accessmanager.h>
#include <tracker/tracker.h>
#include <torrent/server.h>
#include <interfaces/blocklistinterface.h>

class TestBlockList : public bt::BlockListInterface
{
public:
    TestBlockList()
    {
        
    }
    
    virtual ~TestBlockList()
    {
        
    }
    
    virtual bool blocked(const net::Address& addr) const
    {
        return addr.toString() == "8.8.8.8";
    }
};

class AccessManagerTest : public QObject
{
    Q_OBJECT
public:
    
    
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog("accessmanagertest.log");
    }
    
    void cleanupTestCase()
    {
    }
    
    void testCustomIP()
    {
        bt::Tracker::setCustomIP("123.123.123.123");
        bt::Server::setPort(7777);
        QVERIFY(!bt::AccessManager::instance().allowed(net::Address("123.123.123.123", 7777)));
        QVERIFY(bt::AccessManager::instance().allowed(net::Address("123.123.123.123", 7776)));
    }
    
    void testExternalAddress()
    {
        bt::Server::setPort(7777);
        bt::AccessManager::instance().addExternalIP("12.12.12.12");
        QVERIFY(!bt::AccessManager::instance().allowed(net::Address("12.12.12.12", 7777)));
        QVERIFY(bt::AccessManager::instance().allowed(net::Address("12.12.12.12", 7776)));
    }
    
    
    
    void testBlockList()
    {
        bt::AccessManager::instance().addBlockList(new TestBlockList());
        QVERIFY(!bt::AccessManager::instance().allowed(net::Address("8.8.8.8", 7777)));
        QVERIFY(bt::AccessManager::instance().allowed(net::Address("8.8.8.9", 7776)));
    }
    
private:
    
};

QTEST_MAIN(AccessManagerTest)

#include "accessmanagertest.moc"

