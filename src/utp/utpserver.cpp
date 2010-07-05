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
			sock(0),
			running(false),
			utp_thread(0),
			mutex(QMutex::Recursive),
			create_sockets(true),
			tos(0),
			read_notifier(0),
			write_notifier(0),
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
		
		qDeleteAll(pending);
		pending.clear();
		delete mtc;
	}

	void UTPServer::Private::sendOutputQueue()
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
				int ret = sock->sendTo((const bt::Uint8*)data.data(),data.size(),addr);
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
		
		write_notifier->setEnabled(!output_queue.empty());
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
		
		timer.stop();
		
		// Cleanup all connections
		QList<UTPSocket*> sockets;
		bt::PtrMap<Connection*,UTPSocket>::iterator i = alive_connections.begin();
		while (i != alive_connections.end())
		{
			sockets.append(i->second);
			i++;
		}
		
		foreach (UTPSocket* s,sockets)
			s->reset();
		
		alive_connections.clear();
		clearDeadConnections();
		
		connections.setAutoDelete(true);
		connections.clear();
		connections.setAutoDelete(false);
		
		// Close the socket
		if (sock)
		{
			sock->close();
			delete sock;
			sock = 0;
			Globals::instance().getPortList().removePort(port,net::UDP);
		}
	}
	
	bool UTPServer::Private::bind(const net::Address& addr)
	{
		if (sock)
		{
			Globals::instance().getPortList().removePort(port,net::UDP);
			delete sock;
		}
		
		sock = new net::Socket(false,addr.ipVersion());
		sock->setBlocking(false);
		if (!sock->bind(addr,false))
		{
			delete sock;
			sock = 0;
			return false;
		}
		else
		{
			Out(SYS_UTP|LOG_NOTICE) << "UTP: bound to " << addr.toString() << endl;
			sock->setTOS(tos);
			Globals::instance().getPortList().addNewPort(addr.port(),net::UDP,true);
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
	
	void UTPServer::Private::wakeUpPollPipes()
	{
		bool restart_timer = false;
		QMutexLocker lock(&mutex);
		for (PollPipePairItr itr = poll_pipes.begin();itr != poll_pipes.end();itr++)
		{
			PollPipePair* pp = itr->second;
			if (pp->read_pipe->polling())
				itr->second->testRead(connections.begin(),connections.end());
			if (pp->write_pipe->polling())
				itr->second->testWrite(connections.begin(),connections.end());
			
			restart_timer = restart_timer || pp->read_pipe->polling() || pp->write_pipe->polling();
		}
		
		if (restart_timer)
			timer.start(10,p);
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
		QMutexLocker lock(&d->mutex);
		foreach (mse::StreamSocket* s,d->pending)
		{
			newConnection(s);
		}
		
		d->pending.clear();
	}

	
	bool UTPServer::changePort(bt::Uint16 p)
	{
		if (d->sock && port == p)
			return true;
		
		QStringList possible = bindAddresses();
		foreach (const QString & ip,possible)
		{
			net::Address addr(ip,p);
			if (d->bind(addr))
				return true;
		}
		
		return false;
	}
	
	void UTPServer::setTOS(Uint8 type_of_service)
	{
		d->tos = type_of_service;
		if (d->sock)
			d->sock->setTOS(d->tos);
	}
	
	void UTPServer::threadStarted()
	{
		if (!d->read_notifier)
		{
			d->read_notifier = new QSocketNotifier(d->sock->fd(),QSocketNotifier::Read,this);
			connect(d->read_notifier,SIGNAL(activated(int)),this,SLOT(readPacket(int)));
		}
		
		if (!d->write_notifier)
		{
			d->write_notifier = new QSocketNotifier(d->sock->fd(),QSocketNotifier::Write,this);
			connect(d->write_notifier,SIGNAL(activated(int)),this,SLOT(writePacket(int)));
		}
		
		d->write_notifier->setEnabled(false);
	}
	
	void UTPServer::readPacket(int)
	{
		QMutexLocker lock(&d->mutex);
		
		static bt::Uint8 packet_buf[MAX_PACKET_SIZE];
		net::Address addr;
		int ret = d->sock->recvFrom(packet_buf,MAX_PACKET_SIZE,addr);
		if (ret > 0)
		{
			//Out(SYS_UTP|LOG_NOTICE) << "UTP: received " << ba << " bytes packet from " << addr.toString() << endl;
			
			try
			{
				if (ret >= (int)utp::Header::size()) // discard packets which are to small
				{
					handlePacket(QByteArray::fromRawData((const char*)packet_buf,ret),addr);
				}
			}
			catch (utp::Connection::TransmissionError & err)
			{
				Out(SYS_UTP|LOG_NOTICE) << "UTP: " << err.location << endl;
			}
		}
	}
	
	void UTPServer::writePacket(int)
	{
		d->sendOutputQueue();
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
		
		switch (hdr->type)
		{
			case ST_DATA:
			case ST_FIN:
			case ST_STATE:
				try
				{
					Connection* c = d->find(hdr->connection_id);
					if (c)
						c->handlePacket(parser,packet);
				}
				catch (Connection::TransmissionError & err)
				{
					Out(SYS_UTP|LOG_NOTICE) << "UTP: " << err.location << endl;
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
			int ret = d->sock->sendTo((const bt::Uint8*)data.data(),data.size(),addr);
			if (ret == net::SEND_WOULD_BLOCK)
				d->output_queue.append(OutputQueueEntry(data,addr,conn_id));
			else if (ret == net::SEND_FAILURE)
				return false;
		}
		else
			d->output_queue.append(OutputQueueEntry(data,addr,conn_id));
		
		return true;
	}

	Connection* UTPServer::connectTo(const net::Address& addr)
	{
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
			d->utp_thread->start();
		}
	}
	
	void UTPServer::timerEvent(QTimerEvent* event)
	{
		if (event->timerId() == d->timer.timerId())
			d->wakeUpPollPipes();
		else
			QObject::timerEvent(event);
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
			pair->read_pipe->prepare(p,conn->receiveConnectionID(),pair->read_pipe);
		}
		else
		{
			pair->write_pipe->prepare(p,conn->receiveConnectionID(),pair->write_pipe);
		}
		
		if (!d->timer.isActive())
			d->timer.start(10,this);
	}
	
	void UTPServer::onAccepted(Connection* conn)
	{
		if (d->create_sockets)
		{
			UTPSocket* utps = new UTPSocket(conn);
			mse::StreamSocket* ss = new mse::StreamSocket(utps);
			d->pending.append(ss);
			handlePendingConnectionsDelayed();
		}
	}
	
	///////////////////////////////////////////////////////
	
	PollPipePair::PollPipePair() 
		: read_pipe(new PollPipe(net::Poll::INPUT)),
		write_pipe(new PollPipe(net::Poll::OUTPUT))
	{
		
	}
		
	void PollPipePair::testRead(utp::ConItr b, utp::ConItr e)
	{
		for (utp::ConItr i = b;i != e;i++)
		{
			if (read_pipe->readyToWakeUp(i->second))
			{
				read_pipe->wakeUp();
				break;
			}
		}
	}

	void PollPipePair::testWrite(utp::ConItr b, utp::ConItr e)
	{
		for (utp::ConItr i = b;i != e;i++)
		{
			if (write_pipe->readyToWakeUp(i->second))
			{
				write_pipe->wakeUp();
				break;
			}
		}
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