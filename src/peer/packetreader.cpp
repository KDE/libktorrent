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
#include "packetreader.h"
#include <QtAlgorithms>
#include <util/log.h>
#include <util/file.h>
#include <util/functions.h>
#include "peer.h"


namespace bt
{

	IncomingPacket::IncomingPacket(Uint32 size) : data(new Uint8[size]), size(size), read(0)
	{
	}

	PacketReader::PacketReader(Uint32 max_packet_size)
			: error(false), len_received(-1), max_packet_size(max_packet_size)
	{
	}


	PacketReader::~PacketReader()
	{
	}


	IncomingPacket::Ptr PacketReader::dequeuePacket()
	{
		QMutexLocker lock(&mutex);
		if (packet_queue.size() == 0)
			return IncomingPacket::Ptr();

		IncomingPacket::Ptr pck = packet_queue.front();
		if (pck->read != pck->size)
			return IncomingPacket::Ptr();

		packet_queue.pop_front();
		return pck;
	}


	void PacketReader::update(PeerInterface & peer)
	{
		if (error)
			return;

		IncomingPacket::Ptr pck = dequeuePacket();
		while (pck)
		{
			peer.handlePacket(pck->data.data(), pck->size);
			pck = dequeuePacket();
		}
	}

	Uint32 PacketReader::newPacket(Uint8* buf, Uint32 size)
	{
		Uint32 packet_length = 0;
		Uint32 am_of_len_read = 0;
		if (len_received > 0)
		{
			if ((int) size < 4 - len_received)
			{
				memcpy(len + len_received, buf, size);
				len_received += size;
				return size;
			}
			else
			{
				memcpy(len + len_received, buf, 4 - len_received);
				am_of_len_read = 4 - len_received;
				len_received = 0;
				packet_length = ReadUint32(len, 0);

			}
		}
		else if (size < 4)
		{
			memcpy(len, buf, size);
			len_received = size;
			return size;
		}
		else
		{
			packet_length = ReadUint32(buf, 0);
			am_of_len_read = 4;
		}

		if (packet_length == 0)
			return am_of_len_read;

		if (packet_length > max_packet_size)
		{
			Out(SYS_CON | LOG_DEBUG) << " packet_length too large " << packet_length << endl;
			error = true;
			return size;
		}

		IncomingPacket::Ptr pck(new IncomingPacket(packet_length));
		packet_queue.push_back(pck);
		return am_of_len_read + readPacket(buf + am_of_len_read, size - am_of_len_read);
	}

	Uint32 PacketReader::readPacket(Uint8* buf, Uint32 size)
	{
		if (!size)
			return 0;

		IncomingPacket::Ptr pck = packet_queue.back();
		if (pck->read + size >= pck->size)
		{
			// we can read the full packet
			Uint32 tr = pck->size - pck->read;
			memcpy(pck->data.data() + pck->read, buf, tr);
			pck->read += tr;
			return tr;
		}
		else
		{
			// we can do a partial read
			Uint32 tr = size;
			memcpy(pck->data.data() + pck->read, buf, tr);
			pck->read += tr;
			return tr;
		}
	}


	void PacketReader::onDataReady(Uint8* buf, Uint32 size)
	{
		if (error)
			return;

		mutex.lock();
		if (packet_queue.size() == 0)
		{
			Uint32 ret = 0;
			while (ret < size && !error)
			{
				ret += newPacket(buf + ret, size - ret);
			}
		}
		else
		{
			Uint32 ret = 0;
			IncomingPacket::Ptr pck = packet_queue.back();
			if (pck->read == pck->size) // last packet in queue is fully read
				ret = newPacket(buf, size);
			else
				ret = readPacket(buf, size);

			while (ret < size  && !error)
			{
				ret += newPacket(buf + ret, size - ret);
			}
		}
		mutex.unlock();
	}
}
