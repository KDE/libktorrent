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

#include <QObject>
#include <QRandomGenerator>
#include <QtTest>

#include <util/log.h>
#include <utp/connection.h>
#include <utp/utpsocket.h>
#include <utp/utpserver.h>
#include <util/functions.h>

#include <ctime>
#include <unistd.h>


#define PACKETS_TO_SEND 20
#define TEST_DATA "This is the packet loss test\n"

using namespace utp;
using namespace bt;

/**
	Server which simulates packet loss
*/
class PacketLossServer : public UTPServer
{
	Q_OBJECT
public:
	PacketLossServer(QObject* parent = 0) : UTPServer(parent),packet_loss(false),loss_factor(0.5)
	{
		setCreateSockets(false);
	}
	
	~PacketLossServer() override
	{}
	
	
	void handlePacket(bt::Buffer::Ptr packet, const net::Address& addr) override
	{
		if (packet_loss)
		{
			// pick a random number from 0 to 100
			int r = QRandomGenerator::global()->bounded(100);
			if (r <= (int)qRound(100 * loss_factor))
			{
				Out(SYS_UTP|LOG_DEBUG) << "Dropping packet " << endl;
				return;
			}
		}
		
		UTPServer::handlePacket(packet,addr);
	}
	
	void setPacketLossSimulation(bool on,float lf)
	{
		packet_loss = on;
		loss_factor = lf;
	}
	
private:
	bool packet_loss;
	float loss_factor;
};

class SendThread : public QThread
{
	Q_OBJECT
public:
	
	SendThread(Connection::Ptr outgoing,QObject* parent = 0) : QThread(parent),outgoing(outgoing)
	{}
	
	void run() override
	{
		char test[] = TEST_DATA;
		int sent = 0;
		while (sent < PACKETS_TO_SEND && outgoing->connectionState() != CS_CLOSED)
		{
			int ret = outgoing->send((const bt::Uint8*)test,strlen(test));
			if (ret > 0)
			{
				sent++;
			}
			
			msleep(200);
		}
		
		while (!outgoing->allDataSent() && outgoing->connectionState() != CS_CLOSED)
			sleep(1);
	
		Out(SYS_UTP|LOG_DEBUG) << "Transmitted " << sent << " packets " << endl;
		outgoing->dumpStats();
	}
	
	Connection::Ptr outgoing;
};

class PacketLoss : public QEventLoop
{
	Q_OBJECT
public:
	PacketLoss(QObject* parent = 0) : QEventLoop(parent)
	{
	}
	
public Q_SLOTS:
	void accepted()
	{
		incoming = srv.acceptedConnection().toStrongRef();
		exit();
	}
	
	void endEventLoop()
	{
		exit();
	}
	
private Q_SLOTS:
	void initTestCase()
	{
		bt::InitLog("packetlosstest.log");
		
		port = 50000;
		while (port < 60000)
		{
			if (!srv.changePort(port))
				port++;
			else
				break;
		}
		
		srv.start();
	}
	
	void cleanupTestCase()
	{
		srv.stop();
	}
	
	void testConnect()
	{
		net::Address addr("127.0.0.1",port);
		connect(&srv,SIGNAL(accepted()),this,SLOT(accepted()),Qt::QueuedConnection);
		outgoing = srv.connectTo(addr).toStrongRef();
		QVERIFY(outgoing);
		QTimer::singleShot(5000,this,SLOT(endEventLoop())); // use a 5 second timeout
		exec();
		QVERIFY(incoming);
	}
	
	void testPacketLoss()
	{
		bt::Out(SYS_UTP|LOG_DEBUG) << "testPacketLoss" << bt::endl;
		if (outgoing->connectionState() != CS_CONNECTED || incoming->connectionState() != CS_CONNECTED)
		{
			QSKIP("Not Connected",SkipAll);
			return;
		}
		
		srv.setPacketLossSimulation(true,0.1); // Drop 10 % of all packets
		SendThread st(outgoing);
		incoming->setBlocking(true);
		st.start(); // The thread will start sending a whole bunch of data
		int received = 0;
		QString received_data;

		while (received_data.count(TEST_DATA) < PACKETS_TO_SEND)
		{
			bt::Uint32 ba = incoming->bytesAvailable();
			if (ba > 0)
			{
				QByteArray data(ba,0);
				int ret = incoming->recv((bt::Uint8*)data.data(),ba);
				if (ret > 0)
				{
					received_data.append(data);
					received += ret;
				}
			}
			else if (incoming->connectionState() == CS_CLOSED)
			{
				Out(SYS_UTP|LOG_DEBUG) << "Connection closed " << endl;
				break;
			}
			else
			{
				incoming->waitForData(1000);
			}
		}
		
		st.wait();
		Out(SYS_UTP|LOG_DEBUG) << "Received " << received << " bytes:" << endl;
		Out(SYS_UTP|LOG_DEBUG) << received_data << endl;
		incoming->dumpStats();
		QVERIFY(incoming->bytesAvailable() == 0);
		QVERIFY(received_data.count(TEST_DATA) == PACKETS_TO_SEND);
		QVERIFY(outgoing->allDataSent());
	}
	
private:
	
	
private:
	Connection::Ptr incoming;
	Connection::Ptr outgoing;
	PacketLossServer srv;
	int port;
};

QTEST_MAIN(PacketLoss)

#include "packetlosstest.moc"

