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

#include "outputqueue.h"
#include <util/log.h>
#include <net/socket.h>

using namespace bt;

namespace utp
{

	OutputQueue::OutputQueue()
	{
	}

	OutputQueue::~OutputQueue()
	{
	}
	
	int OutputQueue::add(const QByteArray& data, Connection::WPtr conn)
	{
		QMutexLocker lock(&mutex);
		queue.append(Entry(data,conn));
		return queue.size();
	}
	
	void OutputQueue::send(net::ServerSocket* sock)
	{
		QMutexLocker lock(&mutex);
		try
		{
			// Keep sending until the output queue is empty or the socket 
			// can't handle the data anymore
			while (!queue.empty())
			{
				Entry & packet = queue.front();
				Connection::Ptr conn = packet.conn.toStrongRef();
				if (!conn)
				{
					queue.pop_front();
					continue;
				}
				
				int ret = sock->sendTo(packet.data,conn->remoteAddress());
				if (ret == net::SEND_WOULD_BLOCK)
					break;
				else if (ret == net::SEND_FAILURE)
				{
					// Kill the connection of this packet
					conn->close();
					queue.pop_front();
				}
				else
					queue.pop_front();
			}
		}
		catch (Connection::TransmissionError & err)
		{
			Out(SYS_UTP|LOG_NOTICE) << "UTP: " << err.location << endl;
		}
		sock->setWriteNotificationsEnabled(!queue.isEmpty());
	}


}
