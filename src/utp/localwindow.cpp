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

#include "localwindow.h"
#include "utpprotocol.h"
#include <QtAlgorithms>
#include <util/log.h>

using namespace bt;

namespace utp
{

	WindowPacket::WindowPacket(bt::Uint16 seq_nr, bt::Buffer::Ptr packet, bt::Uint32 data_off)
			: seq_nr(seq_nr),
			  packet(packet),
			  bytes_read(data_off)
	{
	}

	WindowPacket::~WindowPacket()
	{
	}

	bt::Uint32 WindowPacket::read(bt::Uint8* dst, bt::Uint32 max_len)
	{
		bt::Uint32 to_read = qMin(packet->size() - bytes_read, max_len);
		if (to_read == 0)
			return 0;

		memcpy(dst, packet->get() + bytes_read, to_read);
		bytes_read += to_read;
		return to_read;
	}

	bool WindowPacket::fullyRead() const
	{
		return bytes_read == packet->size();
	}

	bool operator < (const WindowPacket & a, const WindowPacket & b)
	{
		return SeqNrCmpS(a.seq_nr, b.seq_nr);
	}

	bool operator < (const WindowPacket & a, bt::Uint16 seq_nr)
	{
		return SeqNrCmpS(a.seq_nr, seq_nr);
	}

	bool operator < (bt::Uint16 seq_nr, const WindowPacket & a)
	{
		return  SeqNrCmpS(seq_nr, a.seq_nr);
	}

	LocalWindow::LocalWindow(bt::Uint32 cap)
		: last_seq_nr(0),
		  capacity(cap),
		  window_space(cap),
		  bytes_available(0)
	{

	}

	LocalWindow::~LocalWindow()
	{
	}


	void LocalWindow::setLastSeqNr(bt::Uint16 lsn)
	{
		last_seq_nr = lsn;
	}

	bt::Uint32 LocalWindow::read(bt::Uint8* data, bt::Uint32 max_len)
	{
		WindowPacketList::iterator i = incoming_packets.begin();
		bt::Uint32 written = 0;
		while (i != incoming_packets.end() &&  SeqNrCmpSE(i->seq_nr, last_seq_nr) && written < max_len)
		{
			bt::Uint32 ret = i->read(data + written, max_len - written);
			written += ret;
			window_space += ret;
			bytes_available -= ret;
			if (i->fullyRead())
				i = incoming_packets.erase(i);
			else
				break;
		}

		return written;
	}

	struct FindLowerBound
	{
		FindLowerBound(bt::Uint16 seq_nr) : seq_nr(seq_nr) {}

		bool operator () (const WindowPacket & pkt)
		{
			return SeqNrCmpS(seq_nr, pkt.seq_nr);
		}

		bt::Uint16 seq_nr;
	};

	bool LocalWindow::packetReceived(const utp::Header* hdr, bt::Buffer::Ptr packet, bt::Uint32 data_off)
	{
		// Drop duplicate data packets
		// Make sure we take into account wrapping around
		if (SeqNrCmpSE(hdr->seq_nr, last_seq_nr))
			return true;

		bt::Uint32 data_size = packet->size() - data_off;
		if (availableSpace() < data_size)
		{
			Out(SYS_UTP | LOG_DEBUG) << "Not enough space in local window " << availableSpace() << " " << data_size << endl;
			return false;
		}

		WindowPacketList::iterator itr = std::find_if(incoming_packets.begin(), incoming_packets.end(), FindLowerBound(hdr->seq_nr));
		if (itr == incoming_packets.end())
			incoming_packets.push_back(WindowPacket(hdr->seq_nr, packet, data_off));
		else if (itr->seq_nr == hdr->seq_nr) // already got this one
			return true;
		else
			incoming_packets.insert(itr, WindowPacket(hdr->seq_nr, packet, data_off));

		bt::Uint16 next_seq_nr = last_seq_nr + 1;
		if (hdr->seq_nr == next_seq_nr)
		{
			bytes_available += data_size;
			last_seq_nr = hdr->seq_nr;

			// See if we can increase the last_seq_nr some more
			while (itr != incoming_packets.end())
			{
				next_seq_nr = last_seq_nr + 1;
				if (itr->seq_nr == next_seq_nr)
				{
					bytes_available += itr->packet->size() - itr->bytes_read;
					last_seq_nr = next_seq_nr;
					itr++;
				}
				else
					break;
			}
		}

		window_space -= data_size;
		return true;
	}


	bt::Uint32 LocalWindow::selectiveAckBits() const
	{
		if (!incoming_packets.empty() && SeqNrCmpS(last_seq_nr, incoming_packets.back().seq_nr))
			return SeqNrDiff(last_seq_nr, incoming_packets.back().seq_nr) - 1;
		else
			return 0;
	}

	struct FindUpperBound
	{
		FindUpperBound(bt::Uint16 seq_nr) : seq_nr(seq_nr) {}

		bool operator () (const WindowPacket & pkt)
		{
			return SeqNrCmpSE(seq_nr, pkt.seq_nr);
		}

		bt::Uint16 seq_nr;
	};

	void LocalWindow::fillSelectiveAck(SelectiveAck* sack)
	{
		// First turn off all bits
		memset(sack->bitmask, 0, sack->length);

		WindowPacketList::iterator itr = std::find_if(incoming_packets.begin(), incoming_packets.end(), FindUpperBound(last_seq_nr + 1));
		while (itr != incoming_packets.end())
		{
			Ack(sack, SeqNrDiff(last_seq_nr, itr->seq_nr));
			itr++;
		}
	}

}

