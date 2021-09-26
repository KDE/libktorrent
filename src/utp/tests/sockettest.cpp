/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
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
#include <boost/concept_check.hpp>
#include <torrent/globals.h>
#include <util/log.h>
#include <utp/utpserver.h>
#include <utp/utpsocket.h>

using namespace utp;

class SocketTest : public QEventLoop
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
        bt::InitLog("sockettest.log");
        incoming = outgoing = nullptr;

        port = 50000;
        while (port < 60000) {
            if (bt::Globals::instance().initUTPServer(port))
                break;
            else
                port++;
        }

        bt::Globals::instance().getUTPServer().setCreateSockets(false);
    }

    void cleanupTestCase()
    {
        bt::Globals::instance().shutdownUTPServer();
        delete incoming;
        delete outgoing;
    }

    void testConnect()
    {
        UTPServer &srv = bt::Globals::instance().getUTPServer();
        net::Address addr("127.0.0.1", port);
        connect(&srv, &UTPServer::accepted, this, &SocketTest::accepted, Qt::QueuedConnection);
        outgoing = new UTPSocket();
        outgoing->connectTo(addr);
        QTimer::singleShot(5000, this, &SocketTest::endEventLoop); // use a 5 second timeout
        exec();
        QVERIFY(incoming != nullptr);
        QVERIFY(incoming->connectSuccesFull());
        QVERIFY(outgoing->connectSuccesFull());
    }

    void testSend()
    {
        outgoing->setBlocking(true);
        incoming->setBlocking(true);
        char test[] = "TEST";

        UTPSocket *a = incoming;
        UTPSocket *b = outgoing;
        for (int i = 0; i < 10; i++) {
            int ret = a->send((const bt::Uint8 *)test, strlen(test));
            QVERIFY(ret == (int)strlen(test));

            char tmp[20];
            memset(tmp, 0, 20);
            ret = b->recv((bt::Uint8 *)tmp, 20);
            QVERIFY(ret == 4);
            QVERIFY(memcmp(tmp, test, ret) == 0);
            std::swap(a, b);
        }
    }

    void testClose()
    {
        outgoing->setBlocking(true);
        incoming->close();
        bt::Uint8 tmp[20];
        int ret = outgoing->recv(tmp, 20);
        QVERIFY(ret == 0);
    }

    void testConnectionTimeout()
    {
        UTPSocket sock;
        net::Address addr("127.0.0.1", port + 1);
        sock.setBlocking(true);
        QVERIFY(sock.connectTo(addr) == false);
    }

    void testInvalidAddress()
    {
        UTPSocket sock;
        net::Address addr("127.0.0.1", 0);
        sock.setBlocking(true);
        QVERIFY(sock.connectTo(addr) == false);
    }

private:
    int port;
    utp::UTPSocket *incoming;
    utp::UTPSocket *outgoing;
};

QTEST_MAIN(SocketTest)
