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
#include <util/log.h>
#include <utp/utpserver.h>
#include <utp/connection.h>

using namespace utp;

class ConnectTest : public QEventLoop
{
	Q_OBJECT
public:
	
public Q_SLOTS:
	void accepted()
	{
		accepted_conn = srv.acceptedConnection().toStrongRef();
		exit();
	}
	
	void endEventLoop()
	{
		exit();
	}
	
	void startConnect()
	{
		net::Address addr("127.0.0.1",port);
		srv.connectTo(addr);
		QTimer::singleShot(5000,this,SLOT(endEventLoop())); // use a 5 second timeout
	}
	
private Q_SLOTS:
	void initTestCase()
	{
		bt::InitLog("connecttest.log");
		
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
		QVERIFY(port < 60000);
	}
	
	void cleanupTestCase()
	{
		srv.stop();
	}
	
	void testConnect()
	{
		connect(&srv,SIGNAL(accepted()),this,SLOT(accepted()),Qt::QueuedConnection);
		QTimer::singleShot(0,this,SLOT(startConnect()));
		exec();
		QVERIFY(accepted_conn != 0);
	}
	
private:
	utp::UTPServer srv;
	int port;
	utp::Connection::Ptr accepted_conn;
};

QTEST_MAIN(ConnectTest)

#include "connecttest.moc"
