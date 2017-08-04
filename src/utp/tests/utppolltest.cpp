/***************************************************************************
*   Copyright (C) 2010 by Joris Guisson                                   *
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
#include <unistd.h>
#include <util/log.h>
#include <utp/utpserver.h>
#include <utp/utpsocket.h>
#include <net/poll.h>
#include <torrent/globals.h>
#include <util/bitset.h>

using namespace utp;
using namespace net;
using namespace bt;

#define NUM_SOCKETS 20

class UTPPollTest : public QEventLoop
{
	Q_OBJECT
public:
	
	
public Q_SLOTS:
	void accepted()
	{
		utp::Connection::Ptr conn = bt::Globals::instance().getUTPServer().acceptedConnection();
		incoming[num_accepted++] = new UTPSocket(conn);
		Out(SYS_UTP|LOG_DEBUG) << "Accepted " << num_accepted << endl;
		if (num_accepted >= NUM_SOCKETS)
			exit();
	}
	
	void doConnect()
	{
		for (int i = 0;i < NUM_SOCKETS;i++)
		{
			outgoing[i] = new UTPSocket(); 
			outgoing[i]->setBlocking(false);
			outgoing[i]->connectTo(net::Address("127.0.0.1",port));
		}
	}
	
	void endEventLoop()
	{
		exit();
	}
	
private Q_SLOTS:
	void initTestCase()
	{
		bt::InitLog("utppolltest.log");
		
		port = 50000;
		while (port < 60000)
		{
			if (!bt::Globals::instance().initUTPServer(port))
				port++;
			else
				break;
		}
		
		bt::Globals::instance().getUTPServer().setCreateSockets(false);
		num_accepted = 0;
		for (int i = 0;i < NUM_SOCKETS;i++)
		{
			outgoing[i] = incoming[i] = 0;
		}
	}
	
	void cleanupTestCase()
	{
		bt::Globals::instance().shutdownUTPServer();
		for (int i = 0;i < NUM_SOCKETS;i++)
		{
			delete outgoing[i];
			delete incoming[i];
		}
	}
	
	void testPollConnect()
	{
		poller.reset();
		Out(SYS_UTP|LOG_DEBUG) << "testPollConnect " << endl;
		utp::UTPSocket s;
		s.setBlocking(false);
		s.connectTo(net::Address("127.0.0.1",port));
		s.prepare(&poller,Poll::OUTPUT);
		QVERIFY(poller.poll() > 0);
		QVERIFY(s.ready(&poller,Poll::OUTPUT));
		QVERIFY(s.connectSuccesFull());
		poller.reset();
		
		// Purge accepted connection
		utp::UTPServer & srv = bt::Globals::instance().getUTPServer();
		srv.acceptedConnection();
	}

	void testConnect()
	{
		Out(SYS_UTP|LOG_DEBUG) << "testConnect " << endl;
		utp::UTPServer & srv = bt::Globals::instance().getUTPServer();
		connect(&srv,SIGNAL(accepted()),this,SLOT(accepted()),Qt::QueuedConnection);
		
		QTimer::singleShot(0,this,SLOT(doConnect())); 
		QTimer::singleShot(5000,this,SLOT(endEventLoop())); // use a 5 second timeout
		exec();
		QVERIFY(num_accepted == NUM_SOCKETS);
		for (int i = 0; i < num_accepted; i++)
		{
			Out(SYS_UTP|LOG_DEBUG) << "Check OK incoming " << i << endl;
			QVERIFY(incoming[i]->ok());
		}
	}
	
	void testPollInput()
	{
		Out(SYS_UTP|LOG_DEBUG) << "testPollInput " << endl;
		char test[] = "test\n";
		
		bt::BitSet bs(NUM_SOCKETS);
		poller.reset();
		while (!bs.allOn())
		{
			for (int i = 0;i < NUM_SOCKETS;i++)
			{
				if (!bs.get(i))
					outgoing[i]->prepare(&poller,net::Poll::OUTPUT);
			}
				
			QVERIFY(poller.poll(1000) > 0);
			for (int i = 0;i < NUM_SOCKETS;i++)
			{
				if (bs.get(i) || !outgoing[i]->ready(&poller,net::Poll::OUTPUT))
					continue;
				
				int ret = outgoing[i]->send((const bt::Uint8*)test,strlen(test));
				QVERIFY(ret == (int)strlen(test));
				bs.set(i,true);
			}
		}
		
		bs.setAll(false);
		
		while (!bs.allOn())
		{
			poller.reset();
			for (int i = 0;i < NUM_SOCKETS;i++)
				if (!bs.get(i))
					incoming[i]->prepare(&poller,Poll::INPUT);
			
			Out(SYS_GEN|LOG_DEBUG) << "Entering poll" << endl;
			QVERIFY(poller.poll(1000) > 0);
			for (int i = 0;i < NUM_SOCKETS;i++)
			{
				if (!bs.get(i) && incoming[i]->ready(&poller,net::Poll::INPUT))
				{
					bt::Uint8 tmp[20];
					QVERIFY(incoming[i]->recv(tmp,20) == (int)strlen(test));
					QVERIFY(memcmp(tmp,test,strlen(test)) == 0);
					bs.set(i,true);
				}
			}
		}
		
		poller.reset();
		QVERIFY(bs.allOn());
	}
	
	void testPollOutput()
	{
		poller.reset();
		Out(SYS_UTP|LOG_DEBUG) << "testPollOutput " << endl;
		for (int i = 0;i < NUM_SOCKETS;i++)
		{
			incoming[i]->prepare(&poller,Poll::OUTPUT);
		}
		
		QVERIFY(poller.poll(10000) > 0);
		for (int i = 0;i < NUM_SOCKETS;i++)
		{
			QVERIFY(incoming[i]->ready(&poller,Poll::OUTPUT));
		}
	
		poller.reset();
	}
	
	void testPollClose()
	{
		Out(SYS_UTP|LOG_DEBUG) << "testPollClose " << endl;
		for (int i = 0;i < NUM_SOCKETS;i++)
		{
			incoming[i]->close();
			outgoing[i]->prepare(&poller,net::Poll::INPUT);
		}
		
		QVERIFY(poller.poll() > 0);
		poller.reset();
	}

private:
	int port;
	utp::UTPSocket* outgoing[NUM_SOCKETS];
	utp::UTPSocket* incoming[NUM_SOCKETS];
	int num_accepted;
	Poll poller;
};

QTEST_MAIN(UTPPollTest)

#include "utppolltest.moc"

