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

#ifndef UTP_UTPSERVER_P_H
#define UTP_UTPSERVER_P_H

#include <QObject>
#include <QSocketNotifier>
#include <boost/tuple/tuple.hpp>
#include <net/socket.h>
#include <net/poll.h>
#include <net/serversocket.h>
#include <net/wakeuppipe.h>
#include <util/ptrmap.h>
#include <ktorrent_export.h>
#include "utpsocket.h"
#include "connection.h"
#include "pollpipe.h"
#include "utpserver.h"


namespace utp
{
	class UTPServerThread;
	
	/**
		Utility class used by UTPServer to make sure that ServerInterface::newConnection is called
		from the main thread and not from UTP thread (which is dangerous).
	*/
	class MainThreadCall : public QObject
	{
		Q_OBJECT
	public:
		MainThreadCall(UTPServer* server);
		virtual ~MainThreadCall();
		
	public slots:
		/**
			Calls UTPServer::handlePendingConnections, this should be executed in 
			the main thread.
		*/
		void handlePendingConnections();
		
	private:
		UTPServer* server;
	};
	
	typedef bt::PtrMap<quint16,Connection>::iterator ConItr;
	
	struct PollPipePair
	{
		PollPipe::Ptr read_pipe;
		PollPipe::Ptr write_pipe;
		
		PollPipePair();
		
		void testRead(ConItr b,ConItr e);
		void testWrite(ConItr b,ConItr e);
	};
	
	typedef bt::PtrMap<net::Poll*,PollPipePair>::iterator PollPipePairItr;
	typedef boost::tuple<QByteArray,net::Address,quint16> OutputQueueEntry;
	
	class UTPServer::Private : public net::ServerSocket::DataHandler
	{
	public:
		Private(UTPServer* p);
		~Private();
		
		
		bool bind(const net::Address & addr);
		void syn(const PacketParser & parser,const QByteArray & data,const net::Address & addr);
		void reset(const Header* hdr);
		void clearDeadConnections();
		void wakeUpPollPipes();
		Connection* find(quint16 conn_id);
		void sendOutputQueue(net::ServerSocket* sock);
		void stop();
		virtual void dataReceived(const QByteArray& data, const net::Address& addr);
		virtual void readyToWrite(net::ServerSocket* sock);
		int sendTo(const QByteArray& data, const net::Address& addr);
		void enableWriteNotifications();
		
		
	public:
		UTPServer* p;
		QList<net::ServerSocket::Ptr> sockets;
		QList<net::ServerSocket::Ptr> busy_sockets;
		bool running;
		bt::PtrMap<quint16,Connection> connections;
		QList<Connection*> dead_connections;
		bt::PtrMap<Connection*,UTPSocket> alive_connections;
		UTPServerThread* utp_thread;
		QMutex mutex;
		bt::PtrMap<net::Poll*,PollPipePair> poll_pipes;
		bool create_sockets;
		bt::Uint8 tos;
		QList<OutputQueueEntry> output_queue;
		QBasicTimer timer;
		QList<mse::StreamSocket::Ptr> pending;
		QMutex pending_mutex;
		MainThreadCall* mtc;
		int start_timer_event_type;
	};
}

#endif