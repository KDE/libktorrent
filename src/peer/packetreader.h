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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#ifndef BTPACKETREADER_H
#define BTPACKETREADER_H

#include <qmutex.h>
#include <qlist.h>
#include <net/trafficshapedsocket.h>

namespace bt
{
	class Peer;
	
	struct IncomingPacket
	{
		Uint8* data;
		Uint32 size;
		Uint32 read;
		
		IncomingPacket(Uint32 size);
		virtual ~IncomingPacket();
	};

	/**
	 * Chops up the raw byte stream from a socket into bittorrent packets
	 * @author Joris Guisson
	*/
	class PacketReader : public net::SocketReader
	{
		
	public:
		PacketReader(Peer* peer,Uint32 max_packet_size);
		virtual ~PacketReader();
		
		/**
		 * Push packets to Peer (runs in main thread)
		 */
		void update();
		
		/// Did an error occur
		bool ok() const {return !error;}
		
	private:
		Uint32 newPacket(Uint8* buf,Uint32 size);
		Uint32 readPacket(Uint8* buf,Uint32 size);
		virtual void onDataReady(Uint8* buf,Uint32 size);
		IncomingPacket* dequeuePacket();
		
	private:
		Peer* peer;
		bool error;
		QList<IncomingPacket*> packet_queue;
		QMutex mutex;
		Uint8 len[4];
		int len_received;
		Uint32 max_packet_size;
	};

}

#endif
