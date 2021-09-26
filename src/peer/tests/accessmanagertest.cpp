/*
    SPDX-FileCopyrightText: 2012 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <QObject>
#include <QtTest>
#include <interfaces/blocklistinterface.h>
#include <peer/accessmanager.h>
#include <torrent/server.h>
#include <tracker/tracker.h>
#include <util/log.h>

class TestBlockList : public bt::BlockListInterface
{
public:
    TestBlockList()
    {
    }

    ~TestBlockList() override
    {
    }

    bool blocked(const net::Address &addr) const override
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
