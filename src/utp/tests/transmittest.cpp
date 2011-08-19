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
#include <QFile>
#include <QTextStream>
#include <util/log.h>
#include <utp/connection.h>
#include <utp/utpsocket.h>
#include <utp/utpserver.h>
#include <util/functions.h>
#include <unistd.h>
#include <util/sha1hash.h>
#include <util/sha1hashgen.h>

#define BYTES_TO_SEND 100*1024*1024

using namespace utp;
using namespace bt;

static QByteArray Generate(int size)
{
	QByteArray ba(size,0);
/*	for (int i = 0;i < size;i+=4)
	{
		ba[i] = 'A';
		ba[i+1] = 'B';
		ba[i+2] = 'C';
		ba[i+3] = 'D';
	}
	*/
	for (int i = 0;i < size;i++)
	{
		ba[i] = i % 256;
	}
	return ba;
}

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


class SendThread : public QThread
{
	Q_OBJECT
public:
	
	SendThread(Connection::Ptr outgoing,QObject* parent = 0) : QThread(parent),outgoing(outgoing)
	{}
	
	virtual void run()
	{
		int step = 64*1024;
		QByteArray data = Generate(step);
		bt::SHA1HashGen hgen;
		
		bt::Int64 sent = 0;
		int off = 0;
		while (sent < BYTES_TO_SEND && outgoing->connectionState() != CS_CLOSED)
		{
			int to_send = step - off;
			int ret = outgoing->send((const bt::Uint8*)data.data() + off,to_send);
			if (ret > 0)
			{
				hgen.update((const bt::Uint8*)data.data() + off,ret);
				sent += ret;
				off += ret;
				off = off % step;
				Out(SYS_UTP|LOG_DEBUG) << "Transmitted " << sent << endl;
			}
			else if (ret == 0)
			{
				msleep(50);
			}
			else
			{
				break;
			}
		}
		
		sleep(2);
		Out(SYS_UTP|LOG_DEBUG) << "Transmitted " << sent << endl;
		outgoing->dumpStats();
		outgoing->close();
		sent_hash = hgen.get();
	}
	
	Connection::Ptr outgoing;
	bt::SHA1Hash sent_hash;
};

class TransmitTest : public QEventLoop
{
	Q_OBJECT
public:
	TransmitTest(QObject* parent = 0) : QEventLoop(parent)
	{
	}
	
public slots:
	void accepted()
	{
		incoming = srv.acceptedConnection().toStrongRef();
		exit();
	}
	
	void startConnect()
	{
		net::Address addr("127.0.0.1",port);
		outgoing = srv.connectTo(addr);
	}
	
	void endEventLoop()
	{
		exit();
	}
	
private slots:
	void initTestCase()
	{
		bt::InitLog("transmittest.log",false,true,false);
		
		port = 50000;
		while (port < 60000)
		{
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
		connect(&srv,SIGNAL(accepted()),this,SLOT(accepted()),Qt::QueuedConnection);
		QTimer::singleShot(0,this,SLOT(startConnect())); 
		QTimer::singleShot(5000,this,SLOT(endEventLoop())); // use a 5 second timeout
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
		bt::Out(SYS_UTP|LOG_DEBUG) << "testThreaded" << bt::endl;
		if (outgoing->connectionState() != CS_CONNECTED || incoming->connectionState() != CS_CONNECTED)
		{
			QSKIP("Not Connected",SkipAll);
			return;
		}
		
		bt::SHA1HashGen hgen;
		
		SendThread st(outgoing);
		st.start(); // The thread will start sending a whole bunch of data
		bt::Int64 received = 0;
		int failures = 0;
		net::Poll poller;
		while (received < BYTES_TO_SEND && incoming->connectionState() != CS_CLOSED)
		{
			bt::Uint32 ba = incoming->bytesAvailable();
			if (ba > 0)
			{
				failures = 0;
				QByteArray data(ba,0);
				int to_read = ba;//;qMin<bt::Uint32>(1024,ba);
				int ret = incoming->recv((bt::Uint8*)data.data(),to_read);
				QVERIFY(ret == to_read);
				if (ret > 0)
				{
					hgen.update((bt::Uint8*)data.data(),ret);
					received += ret;
					Out(SYS_UTP|LOG_DEBUG) << "Received " << received << endl;
				}
			}
			else if (incoming->connectionState() != CS_CLOSED)
			{
				incoming->waitForData(1000);
			}
		}
		
		st.wait();
		Out(SYS_UTP|LOG_DEBUG) << "Received " << received << endl;
		incoming->dumpStats();
		QVERIFY(incoming->bytesAvailable() == 0);
		QVERIFY(outgoing->allDataSent());
		QVERIFY(received >= BYTES_TO_SEND);
		
		SHA1Hash rhash = hgen.get();
		Out(SYS_UTP|LOG_DEBUG) << "Received data hash: " << rhash.toString() << endl;
		Out(SYS_UTP|LOG_DEBUG) << "Sent data hash:     " << st.sent_hash.toString() << endl;
		QVERIFY(rhash == st.sent_hash);
		
	}
	
private:
	
	
private:
	Connection::Ptr incoming;
	Connection::Ptr outgoing;
	utp::UTPServer srv;
	int port;
};

QTEST_MAIN(TransmitTest)

#include "transmittest.moc"
