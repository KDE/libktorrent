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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#ifndef NETBUFFEREDSOCKET_H
#define NETBUFFEREDSOCKET_H

#include <deque>
#include <QMutex>
#include <net/socket.h>
#include <download/request.h>
#include <download/packet.h>
#include <net/trafficshapedsocket.h>

namespace net
{
	using bt::Uint8;
	using bt::Uint32;
	
	


	/**
	 * @author Joris Guisson <joris.guisson@gmail.com>
	 * 
	 * Extends the TrafficShapedSocket with outbound bittorrent 
	 * packet queues.
	 */
	class PacketSocket : public TrafficShapedSocket
	{
	public:
		PacketSocket(SocketDevice* sock);
		PacketSocket(int fd,int ip_version);
		PacketSocket(bool tcp,int ip_version);
		virtual ~PacketSocket();
		
		/**
		 * Add a packet to send
		 * @param packet The Packet to send
		 **/
		void addPacket(bt::Packet::Ptr packet);
		
		
		virtual Uint32 write(Uint32 max, bt::TimeStamp now);
		virtual bool bytesReadyToWrite() const;
		
		/// Get the number of data bytes uploaded
		Uint32 dataBytesUploaded();
 		
 		/**
		 * Clear all pieces we are not in the progress of sending.
		 * @param reject Whether or not to send a reject
		 */
		void clearPieces(bool reject);
		
		/**
		 * Do not send a piece which matches this request.
		 * But only if we are not already sending the piece.
		 * @param req The request
		 * @param reject Whether we can send a reject instead
		 */
		void doNotSendPiece(const bt::Request& req, bool reject);
		
		/// Get the number of pending piece uploads
		Uint32 numPendingPieceUploads() const;
 		
	protected:
		/**
		 * Preprocess the packet data, before it is sent. Default implementation does nothing.
		 * @param packet The packet
		 **/
		virtual void preProcess(bt::Packet::Ptr packet);

	private:
		bt::Packet::Ptr selectPacket();
		
	protected:
		std::deque<bt::Packet::Ptr> control_packets;
		std::deque<bt::Packet::Ptr> data_packets; //NOTE: revert back to lists because of erase() calls?
		bt::Packet::Ptr curr_packet;
		Uint32 ctrl_packets_sent;
		
		Uint32 uploaded_data_bytes;
	};

}

#endif
