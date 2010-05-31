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

#ifndef UTP_UTPSERVER_H
#define UTP_UTPSERVER_H

#include <QThread>
#include <QSocketNotifier>
#include <boost/tuple/tuple.hpp>
#include <net/socket.h>
#include <net/poll.h>
#include <net/wakeuppipe.h>
#include <util/ptrmap.h>
#include <interfaces/serverinterface.h>
#include <ktorrent_export.h>
#include "connection.h"
#include "pollpipe.h"


namespace utp
{
	class UTPServerThread;
	class UTPSocket;
	class UTPServer;
	
	/**
		Utility class used by UTPServer to make sure that ServerInterface::newConnection is called
		from the main thread and not from UTP thread (which is dangerous).
	*/
	class KTORRENT_EXPORT MainThreadCall : public QObject
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

	/**
		Implements the UTP server. It listens for UTP packets and manages all connections.
	*/
	class KTORRENT_EXPORT UTPServer : public bt::ServerInterface,public Transmitter
	{
		Q_OBJECT
	public:
		UTPServer(QObject* parent = 0);
		virtual ~UTPServer();
		
		/// Enabled creating sockets (tests need to have this disabled)
		void setCreateSockets(bool on) {create_sockets = on;}
		
		virtual bool changePort(bt::Uint16 port);
		
		/// Send a packet to some host
		virtual bool sendTo(const QByteArray & data,const net::Address & addr,quint16 conn_id);
		
		/// Setup a connection to a remote address
		Connection* connectTo(const net::Address & addr);
		
		/// Attach a socket to a Connection
		void attach(UTPSocket* socket,Connection* conn);
		
		/// Detach a socket to a Connection
		void detach(UTPSocket* socket,Connection* conn);
		
		/// Start the UTP server
		void start();
		
		/// Stop the UTP server
		void stop();
		
		/// Prepare the server for polling
		void preparePolling(net::Poll* p,net::Poll::Mode mode,Connection* conn);
		
		/// Set the TOS byte
		void setTOS(bt::Uint8 type_of_service);
		
		/// Thread has been started
		void threadStarted();
		
		/**
			Handle newly connected sockets, it starts authentication on them.
			This needs to be called from the main thread.
		*/
		void handlePendingConnections();
		
	protected:
		bool bind(const net::Address & addr);
		virtual void handlePacket(const QByteArray & packet,const net::Address & addr);
		void syn(const PacketParser & parser,const QByteArray & data,const net::Address & addr);
		void reset(const Header* hdr);
		void clearDeadConnections();
		void wakeUpPollPipes();
		Connection* find(quint16 conn_id);
		virtual void timerEvent(QTimerEvent* event);
		
	signals:
		void handlePendingConnectionsDelayed();
		void accepted(Connection* conn);
		
	private slots:
		void onAccepted(Connection* conn);
		void readPacket(int fd);
		void writePacket(int fd);
		
	private:
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
		
	private:
		net::Socket* sock;
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
		QSocketNotifier* read_notifier;
		QSocketNotifier* write_notifier;
		QBasicTimer timer;
		QList<mse::StreamSocket*> pending;
		MainThreadCall* mtc;
	};

}

#endif // UTP_UTPSERVER_H
