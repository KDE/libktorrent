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

#ifndef UTP_PACKETBUFFER_H_
#define UTP_PACKETBUFFER_H_

#include <QSharedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/circularbuffer.h>
#include <util/bufferpool.h>

namespace utp
{
	class Header;

	/**
	 * Special packet buffer for UTP packets
	 **/
	class KTORRENT_EXPORT PacketBuffer
	{
	public:
		PacketBuffer();
		PacketBuffer(const PacketBuffer & buf);
		virtual ~PacketBuffer();

		/**
		 * Is the buffer empty
		 * */
		bool isEmpty() const {return size == 0;}

		/**
		 * Set the packet's header.
		 * @param header Header
		 * @param extension_length Length of the extension header
		 * @return False if there is not enough head room, true otherwise
		 **/
		bool setHeader(const Header & hdr, bt::Uint32 extension_length);

		/// Get a pointer to the extension data
		bt::Uint8* extensionData() {return extension;}

		/**
		 * Fill with data from a circular buffer. This will invalidate already filled in headers.
		 * @param cbuf The buffer
		 * @param to_read Amount to read
		 * @return The amount used as payload
		 **/
		bt::Uint32 fillData(bt::CircularBuffer & cbuf, bt::Uint32 to_read);

		/**
		 * Fill with data from a buffer.
		 * @param data The data to copy from
		 * @param data_size The data size
		 * @return The amount used as payload
		 **/
		bt::Uint32 fillData(const bt::Uint8* data, bt::Uint32 data_size);

		/**
		 * For testing purpoes fill with dummy data.
		 * @param amount Amount to fill
		 * */
		void fillDummyData(bt::Uint32 amount);

		/**
		 * Clear the PacketBufferDataPool
		 **/
		static void clearPool();

		/// Get the data pointer
		const bt::Uint8* data() const {return header;}

		/// Get the buffer size
		bt::Uint32 bufferSize() const {return size;}

		/// Get the size of the payload
		bt::Uint32 payloadSize() const {return payload ? (buffer->get() + MAX_SIZE) - payload: 0;}

		/// Get the amount of headroom (room in front of payload)
		bt::Uint32 headRoom() const {return payload ? payload - buffer->get() : MAX_SIZE;}

		static const bt::Uint32 MAX_SIZE = 1500;

	private:
		bt::Buffer::Ptr buffer;
		bt::Uint8* header;
		bt::Uint8* extension;
		bt::Uint8* payload;
		bt::Uint32 size;

		static bt::BufferPool::Ptr pool;
	};

}

#endif /* PACKETBUFFER_H_ */
