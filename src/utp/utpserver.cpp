/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
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

#include "utpserver.h"
#include "utpserver_p.h"
#include <QEvent>
#include <QHostAddress>
#include <QCoreApplication>
#include <stdlib.h>
#ifndef Q_CC_MSVC
#include <sys/select.h>
#endif
#include <time.h>
#include <util/log.h>
#include <util/constants.h>
#include <mse/streamsocket.h>
#include <torrent/globals.h>
#include <net/portlist.h>
#include "utpprotocol.h"
#include "utpserverthread.h"
#include "utpsocket.h"

#ifdef Q_WS_WIN
#include <util/win32.h>
#endif


using namespace bt;

namespace utp
{
	
	MainThreadCall::MainThreadCall(UTPServer* server) : server(server)
	{
	}
	
	MainThreadCall::~MainThreadCall()
	{
	}
	
	void MainThreadCall::handlePendingConnections()
	{
		server->handlePendingConnections();
	}
	
	///////////////////////////////////////////////////////////
	
	UTPServer::Private::Private(UTPServer* p) : 
			p(p),
			running(false),
			utp_thread(0),
			mutex(QMutex::Recursive),
			create_sockets(true),
			tos(0),
			mtc(new MainThreadCall(p))
	{
		QObject::connect(p,SIGNAL(handlePendingConnectionsDelayed()),
				mtc,SLOT(handlePendingConnections()),Qt::QueuedConnection);
	
		poll_pipes.setAutoDelete(true);
	}
		
	UTPServer::Private::~Private()
	{
		if (running)
			stop();
		
		pending.clear();
		delete mtc;
	}

	void UTPServer::Private::sendOutputQueue(net::ServerSocket* sock)
	{
		QMutexLocker lock(&mutex);
		
		try
		{
			// Keep sending until the output queue is empty or the socket 
			// can't handle the data anymore
			while (!output_queue.empty())
			{
				OutputQueueEntry & packet = output_queue.front();
				const QByteArray & data = packet.get<0>();
				const net::Address & addr = packet.get<1>();
				int ret = sock->sendTo(data,addr);
				if (ret == net::SEND_WOULD_BLOCK)
					break;
				else if (ret == net::SEND_FAILURE)
				{
					// Kill the connection of this packet
					Connection* conn = find(packet.get<2>());
					if (conn)
						conn->close();
					
					output_queue.pop_front();
				}
				else
					output_queue.pop_front();
			}
		}
		catch (Connection::TransmissionError & err)
		{
			Out(SYS_UTP|LOG_NOTICE) << "UTP: " << err.location << endl;
		}
		
		sock->setWriteNotificationsEnabled(!output_queue.empty());
	}
	
	void UTPServer::Private::stop()
	{
		running = false;
		if (utp_thread)
		{
			utp_thread->exit();
			utp_thread->wait();
			delete utp_thread;
			utp_thread = 0;
		}
		
		// Cleanup all connections
		QList<UTPSocket*> attached;
		bt::PtrMap<Connection*,UTPSocket>::iterator i = alive_connections.begin();
		while (i != alive_connections.end())
		{
			attached.append(i->second);
			i++;
		}
		
		foreach (UTPSocket* s,attached)
			s->reset();
		
		alive_connections.clear();
		clearDeadConnections();
		
		connections.setAutoDelete(true);
		connections.clear();
		connections.setAutoDelete(false);
		
		// Close the socket
		sockets.clear();
		Globals::instance().getPortList().removePort(port,net::UDP);
	}
	
	bool UTPServer::Private::bind(const net::Address& addr)
	{
		net::ServerSocket::Ptr sock(new net::ServerSocket(this));
		if (!sock->bind(addr))
		{
			return false;
		}
		else
		{
			Out(SYS_UTP|LOG_NOTICE) << "UTP: bound to " << addr.toString() << endl;
			sock->setTOS(tos);
			sock->setReadNotificationsEnabled(false);
			sock->setWriteNotificationsEnabled(false);
			sockets.append(sock);
			return true;
		}
	}
	
	void UTPServer::Private::syn(const PacketParser & parser, const QByteArray& data, const net::Address & addr)
	{
		const Header* hdr = parser.header();
		quint16 recv_conn_id = hdr->connection_id + 1;
		if (connections.find(recv_conn_id))
		{
			// Send a reset packet if the ID is in use
			Connection conn(recv_conn_id,Connection::INCOMING,addr,p);
			conn.sendReset();
		}
		else
		{
			Connection* conn = new Connection(recv_conn_id,Connection::INCOMING,addr,p);
			try
			{
				conn->handlePacket(parser,data);
				connections.insert(recv_conn_id,conn);
				p->accepted(conn);
			}
			catch (Connection::TransmissionError & err)
			{
				Out(SYS_UTP|LOG_NOTICE) << "UTP: " << err.location << endl;
				connections.erase(recv_conn_id);
				delete conn;
			}
		}
	}
	
	void UTPServer::Private::reset(const utp::Header* hdr)
	{
		Connection* c = find(hdr->connection_id);
		if (c)
		{
			c->reset();
		}
	}
	
	Connection* UTPServer::Private::find(quint16 conn_id)
	{
		return connections.find(conn_id);
	}
	
	void UTPServer::Private::clearDeadConnections()
	{
		QMutexLocker lock(&mutex);
		QList<Connection*>::iterator i = dead_connections.begin();
		while (i != dead_connections.end())
		{
			Connection* conn = *i;
			if (conn->connectionState() == CS_CLOSED)
			{
				connections.erase(conn->receiveConnectionID());
				delete conn;
				i = dead_connections.erase(i);
			}
			else
				i++;
		}
	}
	
	void UTPServer::Private::wakeUpPollPipes(utp::Connection* conn, bool readable, bool writeable)
	{
		QMutexLocker lock(&mutex);
		for (PollPipePairItr itr = poll_pipes.begin();itr != poll_pipes.end();itr++)
		{
			PollPipePair* pp = itr->second;
			if (readable && pp->read_pipe->polling(conn->receiveConnectionID()))
				itr->second->read_pipe->wakeUp();
			if (writeable && pp->write_pipe->polling(conn->receiveConnectionID()))
				itr->second->write_pipe->wakeUp();
		}
	}
	
	
	void UTPServer::Private::dataReceived(const QByteArray& data, const net::Address& addr)
	{
		QMutexLocker lock(&mutex);
		//Out(SYS_UTP|LOG_NOTICE) << "UTP: received " << ba << " bytes packet from " << addr.toString() << endl;
		try
		{
			if (data.size() >= (int)utp::Header::size()) // discard packets which are to small
			{
				p->handlePacket(data,addr);
			}
		}
		catch (utp::Connection::TransmissionError & err)
		{
			Out(SYS_UTP|LOG_NOTICE) << "UTP: " << err.location << endl;
		}
	}
	
	void UTPServer::Private::readyToWrite(net::ServerSocket* sock)
	{
		sendOutputQueue(sock);
	}

	int UTPServer::Private::sendTo(const QByteArray& data, const net::Address& addr)
	{
		busy_sockets.clear();
		foreach (net::ServerSocket::Ptr sock,sockets)
		{
			int ret = sock->sendTo(data,addr);
			if (ret == net::SEND_WOULD_BLOCK)
				busy_sockets.append(sock);
			else if (ret == net::SEND_FAILURE)
				continue;
			else
				return ret;
		}
		
		if (busy_sockets.count() > 0)
			return net::SEND_WOULD_BLOCK;
		else
			return net::SEND_FAILURE;
	}
	
	void UTPServer::Private::enableWriteNotifications()
	{
		foreach (net::ServerSocket::Ptr sock,busy_sockets)
		{
			sock->setWriteNotificationsEnabled(true);
		}
	}
	
	///////////////////////////////////////////////////////////

	UTPServer::UTPServer(QObject* parent) 
		: ServerInterface(parent),d(new Private(this))
		
	{
		qsrand(time(0));
		connect(this,SIGNAL(accepted(Connection*)),this,SLOT(onAccepted(Connection*)));
	}

	UTPServer::~UTPServer()
	{
		delete d;
	}
	
	void UTPServer::handlePendingConnections()
	{
		// This should be called from the main thread
		QList<mse::StreamSocket::Ptr> p;
		{
			QMutexLocker lock(&d->pending_mutex);
			// Copy the pending list and clear it before using it's contents to avoid a deadlock
			p = d->pending;
			d->pending.clear();
		}
		
		foreach (mse::StreamSocket::Ptr s,p)
		{
			newConnection(s);
		}
	}

	
	bool UTPServer::changePort(bt::Uint16 p)
	{
		if (d->sockets.count() > 0 && port == p)
			return true;
		
		Globals::instance().getPortList().removePort(port,net::UDP);
		d->sockets.clear();
		
		QStringList possible = bindAddresses();
		foreach (const QString & addr,possible)
		{
			d->bind(net::Address(addr,p));
		}
		
		if (d->sockets.count() == 0)
		{
			// Try any addresses if previous binds failed
			d->bind(net::Address(QHostAddress(QHostAddress::AnyIPv6).toString(),p));
			d->bind(net::Address(QHostAddress(QHostAddress::Any).toString(),p));
		}
		
		if (d->sockets.count())
		{
			Globals::instance().getPortList().addNewPort(p,net::UDP,true);
			return true;
		}
		else
			return false;
	}
	
	void UTPServer::setTOS(Uint8 type_of_service)
	{
		d->tos = type_of_service;
		foreach (net::ServerSocket::Ptr sock,d->sockets)
			sock->setTOS(d->tos);
	}
	
	void UTPServer::threadStarted()
	{
		foreach (net::ServerSocket::Ptr sock,d->sockets)
		{
			sock->setReadNotificationsEnabled(true);
		}
	}
	
#if 0
	static void Dump(const QByteArray & data, const net::Address& addr)
	{
		Out(SYS_UTP|LOG_DEBUG) << QString("Received packet from %1 (%2 bytes)").arg(addr.toString()).arg(data.size()) << endl;
		const bt::Uint8* pkt = (const bt::Uint8*)data.data();
		
		QString line;
		for (int i = 0;i < data.size();i++)
		{
			if (i > 0 && i % 32 == 0)
			{
				Out(SYS_UTP|LOG_DEBUG) << line << endl;
				line = "";
			}
			
			uint val = pkt[i];
			line += QString("%1").arg(val,2,16,QChar('0'));
			if (i + 1 % 4)
				line += ' ';
		}
		Out(SYS_UTP|LOG_DEBUG) << line << endl;
	}

	static void DumpPacket(const Header & hdr)
	{
		Out(SYS_UTP|LOG_NOTICE) << "==============================================" << endl;
		Out(SYS_UTP|LOG_NOTICE) << "UTP: Packet Header: " << endl;
		Out(SYS_UTP|LOG_NOTICE) << "type:                              " << TypeToString(hdr.type) << endl;
		Out(SYS_UTP|LOG_NOTICE) << "version:                           " << hdr.version << endl;
		Out(SYS_UTP|LOG_NOTICE) << "extension:                         " << hdr.extension << endl;
		Out(SYS_UTP|LOG_NOTICE) << "connection_id:                     " << hdr.connection_id << endl;
		Out(SYS_UTP|LOG_NOTICE) << "timestamp_microseconds:            " << hdr.timestamp_microseconds << endl;
		Out(SYS_UTP|LOG_NOTICE) << "timestamp_difference_microseconds: " << hdr.timestamp_difference_microseconds << endl;
		Out(SYS_UTP|LOG_NOTICE) << "wnd_size:                          " << hdr.wnd_size << endl;
		Out(SYS_UTP|LOG_NOTICE) << "seq_nr:                            " << hdr.seq_nr << endl;
		Out(SYS_UTP|LOG_NOTICE) << "ack_nr:                            " << hdr.ack_nr << endl;
		Out(SYS_UTP|LOG_NOTICE) << "==============================================" << endl;
	}
#endif

	void UTPServer::handlePacket(const QByteArray& packet, const net::Address& addr)
	{
		PacketParser parser(packet);
		if (!parser.parse())
			return;
		
		const Header* hdr = parser.header();
		//Dump(packet,addr);
		//DumpPacket(*hdr);
		Connection* c = 0;
		switch (hdr->type)
		{
			case ST_DATA:
			case ST_FIN:
			case ST_STATE:
				try
				{
					c = d->find(hdr->connection_id);
					if (c)
						c->handlePacket(parser,packet);
				}
				catch (Connection::TransmissionError & err)
				{
					Out(SYS_UTP|LOG_NOTICE) << "UTP: " << err.location << endl;
					c->close();
				}
				break;
			case ST_RESET:
				d->reset(hdr);
				break;
			case ST_SYN:
				d->syn(parser,packet,addr);
				break;
		}
	}


	bool UTPServer::sendTo(const QByteArray& data, const net::Address& addr,quint16 conn_id)
	{
		// if output_queue is not empty append to it, so that packet order is OK
		// (when they are being sent anyway)
		if (d->output_queue.empty())
		{
			int ret = d->sendTo(data,addr);
			if (ret == net::SEND_WOULD_BLOCK)
			{
				d->output_queue.append(OutputQueueEntry(data,addr,conn_id));
				d->enableWriteNotifications();
			}
			else if (ret == net::SEND_FAILURE)
				return false;
		}
		else
			d->output_queue.append(OutputQueueEntry(data,addr,conn_id));
		
		return true;
	}

	Connection* UTPServer::connectTo(const net::Address& addr)
	{
		if (d->sockets.isEmpty())
			return 0;
		
		QMutexLocker lock(&d->mutex);
		quint16 recv_conn_id = qrand() % 32535;
		while (d->connections.contains(recv_conn_id))
			recv_conn_id = qrand() % 32535;
		
		Connection* conn = new Connection(recv_conn_id,Connection::OUTGOING,addr,this);
		conn->moveToThread(d->utp_thread);
		d->connections.insert(recv_conn_id,conn);
		try
		{
			conn->startConnecting();
			return conn;
		}
		catch (Connection::TransmissionError & err)
		{
			d->connections.erase(recv_conn_id);
			delete conn;
			return 0;
		}
	}

	
	
	void UTPServer::attach(UTPSocket* socket, Connection* conn)
	{
		QMutexLocker lock(&d->mutex);
		d->alive_connections.insert(conn,socket);
	}

	void UTPServer::detach(UTPSocket* socket, Connection* conn)
	{
		QMutexLocker lock(&d->mutex);
		UTPSocket* sock = d->alive_connections.find(conn);
		if (sock == socket)
		{
			// given the fact that the socket is gone, we can close it
			try
			{
				conn->close();
			}
			catch (Connection::TransmissionError)
			{
			}
			d->alive_connections.erase(conn);
			d->dead_connections.append(conn);
		}
	}

	void UTPServer::stop()
	{
		d->stop();
	}
	
	void UTPServer::start()
	{
		if (!d->utp_thread)
		{
			d->utp_thread = new UTPServerThread(this);
			foreach (net::ServerSocket::Ptr sock,d->sockets)
				sock->moveToThread(d->utp_thread);
			d->utp_thread->start();
		}
	}
	
	void UTPServer::preparePolling(net::Poll* p, net::Poll::Mode mode,Connection* conn)
	{
		QMutexLocker lock(&d->mutex);
		PollPipePair* pair = d->poll_pipes.find(p);
		if (!pair)
		{
			pair = new PollPipePair();
			d->poll_pipes.insert(p,pair);
		}
		
		if (mode == net::Poll::INPUT)
		{
			if (conn->bytesAvailable() > 0 || conn->connectionState() == CS_CLOSED)
				pair->read_pipe->wakeUp();
			pair->read_pipe->prepare(p,conn->receiveConnectionID(),pair->read_pipe);
		}
		else
		{
			if (conn->isWriteable())
				pair->write_pipe->wakeUp();
			pair->write_pipe->prepare(p,conn->receiveConnectionID(),pair->write_pipe);
		}
	}
	
	void UTPServer::stateChanged(Connection* conn, bool readable, bool writeable)
	{
		d->wakeUpPollPipes(conn,readable,writeable);
	}
	
	void UTPServer::onAccepted(Connection* conn)
	{
		if (d->create_sockets)
		{
			UTPSocket* utps = new UTPSocket(conn);
			mse::StreamSocket::Ptr ss(new mse::StreamSocket(utps));
			{
				QMutexLocker lock(&d->pending_mutex);
				d->pending.append(ss);
			}
			handlePendingConnectionsDelayed();
		}
	}
	
	///////////////////////////////////////////////////////
	
	PollPipePair::PollPipePair() 
		: read_pipe(new PollPipe(net::Poll::INPUT)),
		write_pipe(new PollPipe(net::Poll::OUTPUT))
	{
		
	}
		
	bool PollPipePair::testRead(utp::ConItr b, utp::ConItr e)
	{
		for (utp::ConItr i = b;i != e;i++)
		{
			if (read_pipe->readyToWakeUp(i->second))
			{
				read_pipe->wakeUp();
				return true;
			}
		}
		
		return false;
	}

	bool PollPipePair::testWrite(utp::ConItr b, utp::ConItr e)
	{
		for (utp::ConItr i = b;i != e;i++)
		{
			if (write_pipe->readyToWakeUp(i->second))
			{
				write_pipe->wakeUp();
				return true;
			}
		}
		
		return false;
	}
	
	void UTPServer::cleanup()
	{
		d->clearDeadConnections();
	}
	
	void UTPServer::setCreateSockets(bool on)
	{
		d->create_sockets = on;
	}


}