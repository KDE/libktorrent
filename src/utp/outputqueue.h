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


#ifndef UTP_OUTPUTQUEUE_H
#define UTP_OUTPUTQUEUE_H

#include <QList>
#include <QByteArray>
#include <net/serversocket.h>
#include "connection.h"


namespace utp 
{
	/**
	 * Manages the send queue of all UTP server sockets
	 */
	class OutputQueue
	{
	public:
		OutputQueue();
		virtual ~OutputQueue();
		
		/**
		 * Add an entry to the queue.
		 * @param data The packet
		 * @param conn The connection this packet belongs to
		 * @return The number of queued packets
		 */
		int add(const QByteArray & data, Connection::WPtr conn);
		
		/**
		 * Attempt to send the queue on a socket
		 * @param sock The socket
		 */
		void send(net::ServerSocket* sock);
		
	private:
		struct Entry
		{
			QByteArray data;
			Connection::WPtr conn;
			
			Entry(const QByteArray & data, Connection::WPtr conn)
			: data(data),conn(conn)
			{}
		};
		
		QList<Entry> queue;
		QMutex mutex;
	};

}

#endif // UTP_OUTPUTQUEUE_H
