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
		Private(ConnectionHandler* chandler) : sock(0),sn(0),chandler(chandler),dhandler(0)
		{}
		
		Private(DataHandler* dhandler) : sock(0),sn(0),chandler(0),dhandler(dhandler)
		{}
		
		~Private()
		{
			delete sn;
			delete sock;
		}
		
		void reset()
		{
			delete sn;
			sn = 0;
			delete sock;
			sock = 0;
		}
		
		bool isTCP() const {return chandler != 0;}
		
		net::Socket* sock;
		QSocketNotifier* sn;
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
		d->reset();
		
		if (ip.contains(":")) // IPv6
			d->sock = new net::Socket(d->isTCP(),6);
		else
			d->sock = new net::Socket(d->isTCP(),4);
		
		if (d->sock->bind(ip,port,d->isTCP()))
		{
			Out(SYS_GEN|LOG_NOTICE) << "Bound to " << ip << ":" << port << endl;
			d->sock->setBlocking(false);
			d->sn = new QSocketNotifier(d->sock->fd(),QSocketNotifier::Read,this);
			if (d->isTCP())
				connect(d->sn,SIGNAL(activated(int)),this,SLOT(readyToAccept(int)));
			else
				connect(d->sn,SIGNAL(activated(int)),this,SLOT(readyToRead(int)));
			return true;
		}
		else
			d->reset();
		
		return true;
	}
	
	void ServerSocket::readyToAccept(int)
	{
		net::Address addr;
		int fd = d->sock->accept(addr);
		if (fd >= 0)
			d->chandler->newConnection(fd,addr);
	}

	void ServerSocket::readyToRead(int fd)
	{
		net::Address addr;
		QByteArray data(2048,0);
		int ret = d->sock->recvFrom((bt::Uint8*)data.data(),data.size(),addr);
		if (ret > 0)
		{
			data.truncate(ret);
			d->dhandler->dataReceived(data,addr);
		}
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



}

