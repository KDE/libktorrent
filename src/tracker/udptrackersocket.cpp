/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
#include "udptrackersocket.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <QHostAddress>
#include <KLocale>
#include <util/array.h>
#include <net/portlist.h>
#include <util/log.h>
#include <util/functions.h>
#include <net/socket.h>
#include <net/serversocket.h>
#include <torrent/globals.h>
		
#ifdef ERROR
#undef ERROR
#endif



namespace bt
{
	Uint16 UDPTrackerSocket::port = 4444;
	
	class UDPTrackerSocket::Private : public net::ServerSocket::DataHandler
	{
	public:
		Private(UDPTrackerSocket* p) : p(p)
		{
		}
		
		~Private()
		{
		}
		
		void listen(const QString & ip,Uint16 port)
		{
			net::ServerSocket::Ptr sock(new net::ServerSocket(this));
			if (sock->bind(ip,port))
				sockets.append(sock);
		}
		
		bool send(const Uint8* buf,int size,const net::Address & addr)
		{
			foreach (net::ServerSocket::Ptr sock,sockets)
				if (sock->sendTo(buf,size,addr) == size)
					return true;
				
			return false;
		}
		
		virtual void dataReceived(bt::Buffer::Ptr buffer, const net::Address& addr)
		{
			Q_UNUSED(addr);
			if (buffer->size() < 4)
				return;

			Uint32 type = ReadUint32(buffer->get(),0);
			switch (type)
			{
				case CONNECT:
					p->handleConnect(buffer);
					break;
				case ANNOUNCE:
					p->handleAnnounce(buffer);
					break;
				case ERROR:
					p->handleError(buffer);
					break;
				case SCRAPE:
					p->handleScrape(buffer);
					break;
			}
		}
		
		virtual void readyToWrite(net::ServerSocket* sock)
		{
			Q_UNUSED(sock);
		}
		
		
		QList<net::ServerSocket::Ptr> sockets;
		QMap<Int32,Action> transactions;
		UDPTrackerSocket* p;
	};

	UDPTrackerSocket::UDPTrackerSocket() : d(new Private(this))
	{
		if (port == 0)
			port = 4444;
		
		QStringList ips = NetworkInterfaceIPAddresses(NetworkInterface());
		foreach (const QString & ip,ips)
			d->listen(ip,port);
		
		if (d->sockets.count() == 0)
		{
			// Try all addresses if the previous listen calls all failed
			d->listen(QHostAddress(QHostAddress::AnyIPv6).toString(),port);
			d->listen(QHostAddress(QHostAddress::Any).toString(),port);
		}
		
		if (d->sockets.count() == 0)
		{
			Out(SYS_TRK|LOG_IMPORTANT) << QString("Cannot bind to udp port %1").arg(port) << endl;
		}
		else
		{
			Globals::instance().getPortList().addNewPort(port,net::UDP,true);
		}
	}
	
	
	UDPTrackerSocket::~UDPTrackerSocket()
	{
		Globals::instance().getPortList().removePort(port,net::UDP);
		delete d;
	}

	void UDPTrackerSocket::sendConnect(Int32 tid,const net::Address & addr)
	{
		Int64 cid = 0x41727101980LL;
		Uint8 buf[16];

		WriteInt64(buf,0,cid);
		WriteInt32(buf,8,CONNECT);
		WriteInt32(buf,12,tid);
		
		d->send(buf,16,addr);
		d->transactions.insert(tid,CONNECT);
	}

	void UDPTrackerSocket::sendAnnounce(Int32 tid,const Uint8* data,const net::Address & addr)
	{
		d->send(data,98,addr);
		d->transactions.insert(tid,ANNOUNCE);
	}

	void UDPTrackerSocket::sendScrape(Int32 tid, const bt::Uint8* data, const net::Address& addr)
	{
		d->send(data,36,addr);
		d->transactions.insert(tid,SCRAPE);
	}

	void UDPTrackerSocket::cancelTransaction(Int32 tid)
	{
		d->transactions.remove(tid);
	}

	void UDPTrackerSocket::handleConnect(bt::Buffer::Ptr buf)
	{	
		if (buf->size() < 12)
			return;

		// Read the transaction_id and check it
		Int32 tid = ReadInt32(buf->get(),4);
		QMap<Int32,Action>::iterator i = d->transactions.find(tid);
		// if we can't find the transaction, just return
		if (i == d->transactions.end())
			return;

		// check whether the transaction is a CONNECT
		if (i.value() != CONNECT)
		{
			d->transactions.erase(i);
			error(tid,QString());
			return;
		}

		// everything ok, emit signal
		d->transactions.erase(i);
		connectReceived(tid,ReadInt64(buf->get(),8));
	}

	void UDPTrackerSocket::handleAnnounce(bt::Buffer::Ptr buf)
	{
		if (buf->size() < 4)
			return;

		// Read the transaction_id and check it
		Int32 tid = ReadInt32(buf->get(),4);
		QMap<Int32,Action>::iterator i = d->transactions.find(tid);
		// if we can't find the transaction, just return
		if (i == d->transactions.end() || buf->size() < 20)
			return;

		// check whether the transaction is a ANNOUNCE
		if (i.value() != ANNOUNCE)
		{
			d->transactions.erase(i);
			error(tid,QString());
			return;
		}

		// everything ok, emit signal
		d->transactions.erase(i);
		announceReceived(tid,buf->get(), buf->size());
	}
	
	void UDPTrackerSocket::handleError(bt::Buffer::Ptr buf)
	{
		if (buf->size() < 4)
			return;

		// Read the transaction_id and check it
		Int32 tid = ReadInt32(buf->get(),4);
		QMap<Int32,Action>::iterator it = d->transactions.find(tid);
		// if we can't find the transaction, just return
		if (it == d->transactions.end())
			return;

		// extract error message
		d->transactions.erase(it);
		QString msg;
		for (Uint32 i = 8;i < buf->size();i++)
			msg += (char)buf->get()[i];

		// emit signal
		error(tid,msg);
	}

	void UDPTrackerSocket::handleScrape(bt::Buffer::Ptr buf)
	{
		if (buf->size() < 4)
			return;

		// Read the transaction_id and check it
		Int32 tid = ReadInt32((Uint8*)buf.data(),4);
		QMap<Int32,Action>::iterator i = d->transactions.find(tid);
		// if we can't find the transaction, just return
		if (i == d->transactions.end())
			return;
		
		// check whether the transaction is a SCRAPE
		if (i.value() != SCRAPE)
		{
			d->transactions.erase(i);
			error(tid,QString());
			return;
		}
		
		// everything ok, emit signal
		d->transactions.erase(i);
		scrapeReceived(tid,buf->get(),buf->size());
	}

	Int32 UDPTrackerSocket::newTransactionID()
	{
		Int32 transaction_id = rand() * time(0);
		while (d->transactions.contains(transaction_id))
			transaction_id++;
		return transaction_id;
	}

	void UDPTrackerSocket::setPort(Uint16 p)
	{
		port = p;
	}
	
	Uint16 UDPTrackerSocket::getPort()
	{
		return port;
	}
}

