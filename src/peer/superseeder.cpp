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

#include "superseeder.h"
#include "chunkcounter.h"
#include <interfaces/peerinterface.h>

namespace bt
{
	SuperSeeder::SuperSeeder(ChunkCounter* chunk_counter, SuperSeederClient* client)
		: chunk_counter(chunk_counter),client(client)
	{
		for (Uint32 i = 0;i < chunk_counter->getNumChunks();i++)
		{
			if (chunk_counter->get(i) == 0)
				potential_chunks.insert(i);
		}
	}
	
	SuperSeeder::~SuperSeeder()
	{

	}
	
	void SuperSeeder::have(PeerInterface* peer, Uint32 chunk)
	{
		// erase from potential_chunks
		potential_chunks.remove(chunk);
		
		if (active_chunks.contains(chunk))
		{
			// Somebody else has an active chunk we sent to p2
			PeerInterface* p2 = active_chunks[chunk];
			active_chunks.remove(chunk);
			active_peers.remove(p2);
			// p2 has probably been a good boy, he can download another chunk from us
			sendChunk(p2);
		}
	}
	
	void SuperSeeder::haveAll(PeerInterface* peer)
	{
		// Lets just ignore seeders
		if (active_peers.contains(peer))
		{
			Uint32 chunk = active_peers[peer];
			active_chunks.remove(chunk);
			active_peers.remove(peer);
		}
	}
	
	void SuperSeeder::bitset(PeerInterface* peer, const bt::BitSet& bs)
	{
		if (bs.allOn())
		{
			haveAll(peer);
			return;
		}
		
		// Call have for each chunk the peer has
		for (Uint32 i = 0;i < bs.getNumBits();i++)
		{
			if (bs.get(i))
				have(peer,i);
		}
	}
	
	void SuperSeeder::peerAdded(PeerInterface* peer)
	{
		sendChunk(peer);
	}
	
	void SuperSeeder::peerRemoved(PeerInterface* peer)
	{
		// remove the peer
		if (active_peers.contains(peer))
		{
			Uint32 chunk = active_peers[peer];
			active_chunks.remove(chunk);
			active_peers.remove(peer);
		}
	}

	void SuperSeeder::sendChunk(PeerInterface* peer)
	{
		if (active_peers.contains(peer))
			return;
		
		const BitSet & bs = peer->getBitSet();
		foreach (Uint32 chunk,potential_chunks)
		{
			// Skip active chunks and the chunks the peer already has
			if (active_chunks.contains(chunk) || bs.get(chunk))
				continue;
			
			client->allowChunk(peer,chunk);
			active_chunks[chunk] = peer;
			active_peers[peer] = chunk;
			break;
		}
	}

}
