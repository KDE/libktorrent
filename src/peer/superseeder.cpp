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
#include <util/log.h>
#include <interfaces/peerinterface.h>

namespace bt
{
	SuperSeeder::SuperSeeder(Uint32 num_chunks)
		: chunk_counter(new ChunkCounter(num_chunks)),num_seeders(0)
	{
	}
	
	SuperSeeder::~SuperSeeder()
	{
	}
	
	void SuperSeeder::have(PeerInterface* peer, Uint32 chunk)
	{
		chunk_counter->inc(chunk);
		if (peer->getBitSet().allOn()) // it is possible the peer has become a seeder
			num_seeders++;
		
		QList<PeerInterface*> peers;
		
		ActiveChunkItr i = active_chunks.find(chunk);
		while (i != active_chunks.end() && i.key() == chunk)
		{
			// Somebody else has an active chunk we sent to p2
			PeerInterface* p2 = i.value();
			if (peer != p2)
			{
				active_peers.remove(p2);
				i = active_chunks.erase(i);
				// p2 has probably been a good boy, he can download another chunk from us
				peers.append(p2);
			}
			else
				++i;
		}
		
		for (PeerInterface* p: qAsConst(peers))
		{
			sendChunk(p);
		}
	}
	
	void SuperSeeder::haveAll(PeerInterface* peer)
	{
		// Lets just ignore seeders
		if (active_peers.contains(peer))
		{
			Uint32 chunk = active_peers[peer];
			active_chunks.remove(chunk,peer);
			active_peers.remove(peer);
		}
		
		num_seeders++;
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
		if (peer->getBitSet().allOn())
		{
			num_seeders++;
		}
		else
		{
			chunk_counter->incBitSet(peer->getBitSet());
			sendChunk(peer);
		}
	}
	
	void SuperSeeder::peerRemoved(PeerInterface* peer)
	{
		// remove the peer
		if (active_peers.contains(peer))
		{
			Uint32 chunk = active_peers[peer];
			active_chunks.remove(chunk,peer);
			active_peers.remove(peer);
		}
		
		// decrease num_seeders if the peer is a seeder
		if (peer->getBitSet().allOn() && num_seeders > 0)
			num_seeders--;
		
		chunk_counter->decBitSet(peer->getBitSet());
	}

	void SuperSeeder::sendChunk(PeerInterface* peer)
	{
		if (active_peers.contains(peer))
			return;
		
		const BitSet & bs = peer->getBitSet();
		if (bs.allOn())
			return;
		
		// Use random chunk to start searching for a potential chunk we can send
		Uint32 num_chunks = chunk_counter->getNumChunks();
		Uint32 start = qrand() % num_chunks;
		Uint32 chunk = (start + 1) % num_chunks;
		Uint32 alternative = num_chunks;
		while (chunk != start) 
		{
			chunk = (chunk + 1) % num_chunks;
			if (bs.get(chunk))
				continue;
			
			// If we can find a chunk which nobody has, use that
			// keep track of alternatives, if none is found
			if (chunk_counter->get(chunk) - num_seeders == 0)
			{
				peer->chunkAllowed(chunk);
				active_chunks.insert(chunk,peer);
				active_peers[peer] = chunk;
				return;
			}
			else if (alternative == num_chunks)
			{
				alternative = chunk;
			}
		} 
		
		if (alternative < num_chunks)
		{
			peer->chunkAllowed(chunk);
			active_chunks.insert(chunk,peer);
			active_peers[peer] = chunk;
		}
	}
	
	void SuperSeeder::dump()
	{
		Out(SYS_GEN|LOG_DEBUG) << "Active chunks: " << endl;
		ActiveChunkItr i = active_chunks.begin();
		while (i != active_chunks.end()) 
		{
			Out(SYS_GEN|LOG_DEBUG) << "Chunk " << i.key() << " : " << i.value()->getPeerID().toString() << endl;
			++i;
		}
		
		Out(SYS_GEN|LOG_DEBUG) << "Active peers: " << endl;
		ActivePeerItr j = active_peers.begin();
		while (j != active_peers.end())
		{
			Out(SYS_GEN|LOG_DEBUG) << "Peer " << j.key()->getPeerID().toString() << " : " << j.value() << endl;
			++j;
		}
	}


}
