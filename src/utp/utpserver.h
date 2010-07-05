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
#include <interfaces/serverinterface.h>
#include <net/address.h>
#include <net/poll.h>
#include <utp/connection.h>


namespace utp
{
	class UTPSocket;
	
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
		void setCreateSockets(bool on);
		
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
		virtual void handlePacket(const QByteArray & packet,const net::Address & addr);
		virtual void timerEvent(QTimerEvent* event);
		
	signals:
		void handlePendingConnectionsDelayed();
		void accepted(Connection* conn);
		
	private slots:
		void onAccepted(Connection* conn);
		void readPacket(int fd);
		void writePacket(int fd);
		
	public slots:
		void cleanup();
		
	private:
		class Private;
		Private* d;
	};

}

#endif // UTP_UTPSERVER_H
