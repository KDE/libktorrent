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
	WindowPacket::WindowPacket(bt::Uint16 seq_nr)
			: seq_nr(seq_nr),
			  bytes_read(0)
	{

	}

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

	void WindowPacket::set(bt::Buffer::Ptr packet, bt::Uint32 data_off)
	{
		this->packet = packet;
		bytes_read = data_off;
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
		  first_seq_nr(0),
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
		first_seq_nr = lsn;
	}

	bt::Uint32 LocalWindow::read(bt::Uint8* data, bt::Uint32 max_len)
	{
		bt::Uint16 off = SeqNrDiff(incoming_packets.front().seq_nr, first_seq_nr);
		bt::Uint32 written = 0;
		while (off < incoming_packets.size() && incoming_packets[off].packet && SeqNrCmpSE(incoming_packets[off].seq_nr, last_seq_nr) && written < max_len)
		{
			WindowPacket & pkt = incoming_packets[off];
			bt::Uint32 ret = pkt.read(data + written, max_len - written);
			written += ret;
			window_space += ret;
			bytes_available -= ret;
			if (pkt.fullyRead())
			{
				pkt.packet.clear();
				first_seq_nr++;
				off++;
			}
			else
				break;
		}

		// Erase is inefficient, so lets do it when we have a whole bunch to throw away
		off = SeqNrDiff(incoming_packets.front().seq_nr, first_seq_nr);
		if (off > 20)
			incoming_packets.erase(incoming_packets.begin(), incoming_packets.begin() + off);

		return written;
	}

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

		if (incoming_packets.empty())
		{
			first_seq_nr = last_seq_nr + 1;
			for (bt::Uint16 i = last_seq_nr + 1; SeqNrCmpS(i, hdr->seq_nr); i++)
				incoming_packets.push_back(WindowPacket(i));
			incoming_packets.push_back(WindowPacket(hdr->seq_nr, packet, data_off));
		}
		else if (SeqNrCmpS(incoming_packets.back().seq_nr, hdr->seq_nr))
		{
			for (bt::Uint16 i = incoming_packets.back().seq_nr + 1; SeqNrCmpS(i, hdr->seq_nr); i++)
				incoming_packets.push_back(WindowPacket(i));
			incoming_packets.push_back(WindowPacket(hdr->seq_nr, packet, data_off));
		}
		else if (incoming_packets[SeqNrDiff(incoming_packets.front().seq_nr, hdr->seq_nr)].packet)
		{
			// Already got this one
			return true;
		}
		else
		{
			bt::Uint16 off = SeqNrDiff( incoming_packets.front().seq_nr, hdr->seq_nr);
			incoming_packets[off].set(packet, data_off);
		}

		bt::Uint16 next_seq_nr = last_seq_nr + 1;
		if (hdr->seq_nr == next_seq_nr)
		{
			bytes_available += data_size;
			last_seq_nr = hdr->seq_nr;
			next_seq_nr = last_seq_nr + 1;

			bt::Uint16 off = SeqNrDiff(incoming_packets.front().seq_nr, next_seq_nr);
			// See if we can increase the last_seq_nr some more
			while (off < incoming_packets.size())
			{
				WindowPacket & pkt = incoming_packets[off];
				if (pkt.packet)
				{
					bytes_available += pkt.packet->size() - pkt.bytes_read;
					last_seq_nr = next_seq_nr;
					next_seq_nr++;
					off++;
				}
				else
					break;
			}
		}

/*
		Out(SYS_GEN|LOG_DEBUG) << "1 LocalWindow " << bytes_available << " " << last_seq_nr << " " << first_seq_nr << endl;
		for (int i = 0; i < incoming_packets.size(); i++)
			Out(SYS_GEN|LOG_DEBUG) << incoming_packets[i].seq_nr << ": " << (incoming_packets[i].packet ? "OK" : "") << endl;
*/
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

	void LocalWindow::fillSelectiveAck(SelectiveAck* sack)
	{
		// First turn off all bits
		memset(sack->bitmask, 0, sack->length);

		WindowPacketList::iterator itr = std::upper_bound(incoming_packets.begin(), incoming_packets.end(), last_seq_nr + 1);
		while (itr != incoming_packets.end())
		{
			if (itr->packet)
				Ack(sack, SeqNrDiff(last_seq_nr, itr->seq_nr));
			itr++;
		}
	}

}

