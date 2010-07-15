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

#include "serversocket.h"

#include <QSocketNotifier>
#include <util/log.h>
#include "socket.h"

using namespace bt;

namespace net
{
	class ServerSocket::Private
	{
	public:
		Private(ConnectionHandler* chandler) : sock(0),rsn(0),wsn(0),chandler(chandler),dhandler(0)
		{}
		
		Private(DataHandler* dhandler) : sock(0),rsn(0),wsn(0),chandler(0),dhandler(dhandler)
		{}
		
		~Private()
		{
			delete rsn;
			delete wsn;
			delete sock;
		}
		
		void reset()
		{
			delete rsn;
			rsn = 0;
			delete wsn;
			wsn = 0;
			delete sock;
			sock = 0;
		}
		
		bool isTCP() const {return chandler != 0;}
		
		net::Socket* sock;
		QSocketNotifier* rsn;
		QSocketNotifier* wsn;
		ConnectionHandler* chandler;
		DataHandler* dhandler;
	};
	
	ServerSocket::ServerSocket(ConnectionHandler* chandler) : d(new Private(chandler))
	{

	}
	
	ServerSocket::ServerSocket(ServerSocket::DataHandler* dhandler) : d(new Private(dhandler))
	{

	}
	
	ServerSocket::~ServerSocket()
	{
		delete d;
	}
	
	bool ServerSocket::bind(const QString& ip, bt::Uint16 port)
	{
		return bind(net::Address(ip,port));
	}
	
	bool ServerSocket::bind(const net::Address& addr)
	{
		d->reset();
		
		d->sock = new net::Socket(d->isTCP(),addr.ipVersion());
		if (d->sock->bind(addr,d->isTCP()))
		{
			Out(SYS_GEN|LOG_NOTICE) << "Bound to " << addr.toString() << endl;
			d->sock->setBlocking(false);
			d->rsn = new QSocketNotifier(d->sock->fd(),QSocketNotifier::Read,this);
			if (d->isTCP())
			{
				connect(d->rsn,SIGNAL(activated(int)),this,SLOT(readyToAccept(int)));
			}
			else
			{
				d->wsn = new QSocketNotifier(d->sock->fd(),QSocketNotifier::Write,this);
				d->wsn->setEnabled(false);
				connect(d->rsn,SIGNAL(activated(int)),this,SLOT(readyToRead(int)));
				connect(d->wsn,SIGNAL(activated(int)),this,SLOT(readyToWrite(int)));
			}
			return true;
		}
		else
			d->reset();
		
		return false;
	}
	
	void ServerSocket::readyToAccept(int)
	{
		net::Address addr;
		int fd = d->sock->accept(addr);
		if (fd >= 0)
			d->chandler->newConnection(fd,addr);
	}

	void ServerSocket::readyToRead(int)
	{
		net::Address addr;
		static bt::Uint8 tmp_buf[2048];
		int ret = d->sock->recvFrom(tmp_buf,2048,addr);
		if (ret > 0)
		{
			d->dhandler->dataReceived(QByteArray::fromRawData((const char*)tmp_buf,ret),addr);
		}
	}
	
	void ServerSocket::setWriteNotificationsEnabled(bool on)
	{
		if (d->wsn)
			d->wsn->setEnabled(on);
	}
	
	void ServerSocket::setReadNotificationsEnabled(bool on)
	{
		if (d->rsn)
			d->rsn->setEnabled(on);
	}

	
	void ServerSocket::readyToWrite(int)
	{
		d->dhandler->readyToWrite(this);
	}
	
	int ServerSocket::sendTo(const QByteArray& data, const net::Address& addr)
	{
		// Only UDP server socket can send
		if (!d->dhandler)
			return 0;
		
		return d->sock->sendTo((const Uint8*)data.data(),data.size(),addr);
	}
	
	int ServerSocket::sendTo(const bt::Uint8* buf, int size, const net::Address& addr)
	{
		// Only UDP server socket can send
		if (!d->dhandler)
			return 0;
		
		return d->sock->sendTo(buf,size,addr);
	}
	
	bool ServerSocket::setTOS(unsigned char type_of_service)
	{
		if (d->sock)
			return d->sock->setTOS(type_of_service);
		else
			return false;
	}


}

