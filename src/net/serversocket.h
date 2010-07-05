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


#ifndef NET_SERVERSOCKET_H
#define NET_SERVERSOCKET_H

#include <QObject>
#include <QSharedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace net 
{
	class Address;
	

	
	/**
		Convenience class to create and bind a server socket.
		Internally it combines a QSocketNotifier and a net::Socket.
	*/
	class KTORRENT_EXPORT ServerSocket : public QObject
	{
		Q_OBJECT
	public:
		typedef QSharedPointer<ServerSocket> Ptr;
		
		/**
			Interface class to handle new connections
			from a ServerSocket.
		*/
		class KTORRENT_EXPORT ConnectionHandler
		{
		public:
			virtual ~ConnectionHandler() {}
			
			/**
				A new connection has been accepted
				@param fd The filedescriptor of the connection
				@param addr The address of the connection
			*/
			virtual void newConnection(int fd,const net::Address & addr) = 0;
		};
		
		/**
			Interface class to handle data from a ServerSocket
		*/
		class KTORRENT_EXPORT DataHandler
		{
		public:
			virtual ~DataHandler() {}
			
			/**
				An UDP packet was received
				@param data The packet
				@param addr The address from which it was received
			*/
			virtual void dataReceived(const QByteArray & data,const net::Address & addr) = 0;
			
			/**
				Socket has become writeable
				@param sock The socket 
			*/
			virtual void readyToWrite(net::ServerSocket* sock) = 0;
		};
		
		/**
			Create a TCP server socket
			@param chandler The connection handler
		*/
		ServerSocket(ConnectionHandler* chandler);
		
		/**
			Create an UDP server socket
			@param dhandler The data handler
		*/
		ServerSocket(DataHandler* dhandler);
		
		virtual ~ServerSocket();
		
		/**
			Bind the socket to an IP and port
			@param ip The IP address
			@param port The port number
			@return true upon success, false otherwise
		*/
		bool bind(const QString & ip,bt::Uint16 port);
		
		/**
			Bind the socket to an address
			@param addr The address
			@return true upon success, false otherwise
		*/
		bool bind(const net::Address & addr);
		
		/**
			Method to send data with the socket. Only use this when 
			the socket is a UDP socket. It will fail for TCP server sockets.
			@param data The data to send
			@param addr The address to send to
			@return The number of bytes sent
		*/
		int sendTo(const QByteArray & data,const net::Address & addr);
		
		/**
			Method to send data with the socket. Only use this when 
			the socket is a UDP socket. It will fail for TCP server sockets.
			@param buf The data to send
			@param size The size of the data
			@param addr The address to send to
			@return The number of bytes sent
		*/
		int sendTo(const bt::Uint8* buf,int size,const net::Address & addr);
		
		/**
			Enable write notifications.
			@param on On or not
		*/
		void setWriteNotificationsEnabled(bool on);
		
		/**
			Enable read notifications.
			@param on On or not
		*/
		void setReadNotificationsEnabled(bool on);
		
		/**
			Set the TOS byte of the socket
			@param type_of_service Value to set
			@return true upon success, false otherwise
		*/
		bool setTOS(unsigned char type_of_service);
		
	private slots:
		void readyToAccept(int fd);
		void readyToRead(int fd);
		void readyToWrite(int fd);
		
	private:
		class Private;
		Private* d;
	};

}

#endif // NET_SERVERSOCKET_H
