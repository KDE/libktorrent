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

#include "streamingchunkselector.h"
#include <diskio/chunkmanager.h>
#include <interfaces/piecedownloader.h>
#include "downloader.h"

namespace bt
{
	const Uint32 CRITICAL_WINDOW_IN_BYTES = 4*1024*1024;
	const Uint32 INVALID_CHUNK = 0xFFFFFFFF;
	
	
	StreamingChunkSelector::StreamingChunkSelector()
		: range_start(0), range_end(0), cursor(0)
	{
		
	}
	
	StreamingChunkSelector::~StreamingChunkSelector()
	{

	}
	
	void StreamingChunkSelector::init(ChunkManager* cman, Downloader* downer, PeerManager* pman)
	{
		bt::ChunkSelector::init(cman, downer, pman);
		critical_window_size = CRITICAL_WINDOW_IN_BYTES / cman->getTorrent().getChunkSize();
		range_end = cman->getNumChunks() - 1;
	}

	
	void StreamingChunkSelector::setCursor(Uint32 chunk)
	{
		cursor = chunk;
		updateRange();
	}
	
	void StreamingChunkSelector::setSequentialRange(Uint32 from, Uint32 to)
	{
		range_start = from;
		range_end = to;
		cursor = from;
		initRange();
	}
	
	void StreamingChunkSelector::initRange()
	{
		// Initialize the range
		const BitSet & bs = cman->getBitSet();
		range.clear();
		for (Uint32 i = cursor;i <= range_end;i++)
		{
			if (!bs.get(i))
				range.push_back(i);
		}
	}

	
	void StreamingChunkSelector::updateRange()
	{
		const BitSet & bs = cman->getBitSet();
		std::list<Uint32>::iterator itr = range.begin();
		while (itr != range.end())
		{
			Uint32 i = *itr;
			// if we have the chunk or it falls before the cursor remove it from the list
			if (bs.get(i) || i < cursor)
			{
				std::list<Uint32>::iterator tmp = itr;
				itr++;
				range.erase(tmp);
			}
			else
				itr++;
		}
	}


	bool StreamingChunkSelector::select(bt::PieceDownloader* pd, bt::Uint32& chunk)
	{
		const BitSet & bs = cman->getBitSet();
		Uint32 critical_chunk = INVALID_CHUNK;
		Uint32 critical_chunk_downloaders = INVALID_CHUNK;
		Uint32 non_critical_chunk = INVALID_CHUNK;
		
		std::list<Uint32>::iterator itr = range.begin();
		while (itr != range.end())
		{
			Uint32 i = *itr;
			const Chunk* c = cman->getChunk(i);
			
			// if we have the chunk remove it from the list
			if (bs.get(i))
			{
				std::list<Uint32>::iterator tmp = itr;
				itr++;
				range.erase(tmp);
			}
			else if (pd->hasChunk(i) && !c->isExcluded() && !c->isExcludedForDownloading())
			{
				if (i < cursor + critical_window_size)
				{
					// Attempt to find the critical chunk with the least downloaders
					Uint32 nd = downer->numDownloadersForChunk(i);
					if (critical_chunk == INVALID_CHUNK || nd < critical_chunk_downloaders)
					{
						critical_chunk = i;
						critical_chunk_downloaders = nd;
					}
				}
				else if (!downer->downloading(i))
				{
					// Stop at the first non critical chunk
					non_critical_chunk = i;
					break;
				}
				
				itr++;
			}
			else
				itr++;
		}
		
		if (critical_chunk != INVALID_CHUNK)
		{
			chunk = critical_chunk;
			return true;
		}
		else if (non_critical_chunk != INVALID_CHUNK)
		{
			chunk = non_critical_chunk;
			return true;
		}
		else
		{
			// If we haven't found one, use the default selection algorithm
			return bt::ChunkSelector::select(pd, chunk);
		}
	}

	void StreamingChunkSelector::dataChecked(const bt::BitSet& ok_chunks)
	{
		bt::ChunkSelector::dataChecked(ok_chunks);
		updateRange();
	}

	void StreamingChunkSelector::reincluded(bt::Uint32 from, bt::Uint32 to)
	{
		bt::ChunkSelector::reincluded(from, to);
		initRange();
	}

	void StreamingChunkSelector::reinsert(bt::Uint32 chunk)
	{
		bt::ChunkSelector::reinsert(chunk);
		if (chunk >= cursor && chunk <= range_end)
		{
			std::list<Uint32>::iterator itr = range.begin();
			while (itr != range.end())
			{
				Uint32 i = *itr;
				if (i == chunk)
				{
					return;
				}
				if (i > chunk)
				{
					range.insert(itr,chunk);
					return;
				}
				
				itr++;
			}
			
			// Not returned yet, so must be the last chunk in the range
			range.push_back(chunk);
		}
	}

	bool StreamingChunkSelector::selectRange(bt::Uint32& from, bt::Uint32& to, bt::Uint32 max_len)
	{
		return bt::ChunkSelector::selectRange(from, to, max_len);
	}
}
