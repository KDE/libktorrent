/***************************************************************************
 *   Copyright (C) 2011 by Joris Guisson                                   *
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


#ifndef NET_STREAMSOCKET_H
#define NET_STREAMSOCKET_H

#include <QByteArray>
#include <net/trafficshapedsocket.h>


namespace net 
{
	class StreamSocketListener
	{
	public:
		virtual ~StreamSocketListener() {}
		
		/**
		 * Called when a StreamSocket gets connected.
		 */
		virtual void connectFinished(bool succeeded) = 0;
		
		/**
		 * Called when all data has been sent.
		 */
		virtual void dataSent() = 0;
	};
	/**
	 * TrafficShapedSocket which provides a simple buffer as outbound data queue. 
	 * And a callback interface (StreamSocketListener) for notification of events.
	 */
	class StreamSocket : public net::TrafficShapedSocket
	{
	public:
		StreamSocket(bool tcp, int ip_version, StreamSocketListener* listener);
		~StreamSocket() override;
		
		bool bytesReadyToWrite() const override;
		bt::Uint32 write(bt::Uint32 max, bt::TimeStamp now) override;
		
		/**
		 * Add data to send
		 * @param data The QByteArray
		 */
		void addData(const QByteArray & data);
		
	private:
		StreamSocketListener* listener;
		QByteArray buffer;
	};

}

#endif // NET_STREAMSOCKET_H
