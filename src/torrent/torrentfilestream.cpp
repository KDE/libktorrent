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

#include "torrentfilestream.h"
#include <diskio/chunkmanager.h>
#include <diskio/piecedata.h>
#include <util/timer.h>

namespace bt
{
	class TorrentFileStream::Private
	{
	public:
		Private(TorrentInterface* tc,ChunkManager* cman,TorrentFileStream* p);
		Private(TorrentInterface* tc,Uint32 file_index,ChunkManager* cman,TorrentFileStream* p);
		
		void reset();
		void update();
		Uint32 lastChunk();
		Uint32 firstChunk();
		Uint32 firstChunkOffset();
		Uint32 lastChunkSize();
		qint64 readData(char* data, qint64 maxlen);
		qint64 readCurrentChunk(char* data, qint64 maxlen);
		
	public:
		TorrentInterface* tc;
		Uint32 file_index;
		ChunkManager* cman;
		TorrentFileStream* p;
		Uint64 current_byte_offset;
		Uint64 current_limit;
		bool opened;
		
		PieceDataPtr current_chunk_data;
		Uint32 current_chunk;
		Uint32 current_chunk_offset;
		bt::Timer timer;
	};
	
	
	TorrentFileStream::TorrentFileStream(TorrentInterface* tc, ChunkManager* cman,QObject* parent)
		: QIODevice(parent),d(new Private(tc,cman,this))
	{
	}
	
	TorrentFileStream::TorrentFileStream(TorrentInterface* tc, Uint32 file_index, ChunkManager* cman,QObject* parent)
		: QIODevice(parent),d(new Private(tc,file_index,cman,this))
	{
	}

	
	TorrentFileStream::~TorrentFileStream()
	{
		delete d;
	}


	qint64 TorrentFileStream::writeData(const char* data, qint64 len)
	{
		Q_UNUSED(data);
		Q_UNUSED(len);
		return -1;
	}

	qint64 TorrentFileStream::readData(char* data, qint64 maxlen)
	{
		return d->readData(data,maxlen);
	}

	bool TorrentFileStream::open(QIODevice::OpenMode mode)
	{
		if (mode != QIODevice::ReadOnly)
			return false;
	
		QIODevice::open(mode|QIODevice::Unbuffered);
		d->opened = true;
		return true;
	}

	void TorrentFileStream::close()
	{
		d->reset();
		d->opened = false;
	}

	qint64 TorrentFileStream::pos() const
	{
		return d->current_byte_offset;
	}

	qint64 TorrentFileStream::size() const
	{
		if (d->tc->getStats().multi_file_torrent)
		{
			return d->tc->getTorrentFile(d->file_index).getSize();
		}
		else
		{
			return d->tc->getStats().total_bytes;
		}
	}

	bool TorrentFileStream::seek(qint64 pos)
	{
		if (pos < (qint64)d->current_limit)
		{
			d->current_byte_offset = pos;
			return true;
		}
		
		return false;
	}

	bool TorrentFileStream::atEnd() const
	{
		return pos() >= size();
	}

	bool TorrentFileStream::reset()
	{
		if (!d->opened)
			return false;
		
		d->reset();
		return true;
	}

	qint64 TorrentFileStream::bytesAvailable() const
	{
		return d->current_limit - d->current_byte_offset;
	}
	
	//////////////////////////////////////////////////
	TorrentFileStream::Private::Private(TorrentInterface* tc, ChunkManager* cman, TorrentFileStream* p) 
		: tc(tc),file_index(0),cman(cman),p(p),
		current_byte_offset(0),current_limit(0),opened(false),
		current_chunk_offset(0)
	{
		current_chunk = firstChunk();
	}
	
	TorrentFileStream::Private::Private(TorrentInterface* tc, 
										Uint32 file_index, 
										ChunkManager* cman, 
										TorrentFileStream* p)
		: tc(tc),file_index(file_index),cman(cman),p(p),
		current_byte_offset(0),current_limit(0),opened(false),
		current_chunk_offset(0)
	{
		current_chunk = firstChunk();
	}
	
	void TorrentFileStream::Private::reset()
	{
		current_byte_offset = 0;
		current_limit = 0;
		current_chunk = firstChunk();
		current_chunk_offset = 0;
		current_chunk_data.reset();
		cman->checkMemoryUsage();
	}
	
	void TorrentFileStream::Private::update()
	{
		const BitSet & chunks = cman->getBitSet();
		// Update the current limit
		bt::Uint32 first = firstChunk();
		bt::Uint32 last = lastChunk();
		if (first == last)
		{
			// Special case
			if (chunks.get(first))
				current_limit = p->size();
			else
				current_limit = 0;
			return;
		}
		
		// Loop until the end or a chunk we don't have
		Uint32 i = 0;
		for (i = first;i <= last;i++)
		{
			if (!chunks.get(i))
				break;
		}
		
		if (i == first)
		{
			current_limit = 0;
		}
		else
		{
			Uint64 chunk_size = tc->getStats().chunk_size;
			if (i == last + 1)
			{
				// we have the entire file
				current_limit = p->size();
			}
			else
			{
				current_limit = (chunk_size - firstChunkOffset());
				current_limit += (i - first - 1) * chunk_size;
			}
		}
	}
	
	Uint32 TorrentFileStream::Private::firstChunk()
	{
		if (tc->getStats().multi_file_torrent)
			return tc->getTorrentFile(file_index).getFirstChunk();
		else
			return 0;
	}
	
	Uint32 TorrentFileStream::Private::firstChunkOffset()
	{
		if (tc->getStats().multi_file_torrent)
			return tc->getTorrentFile(file_index).getFirstChunkOffset();
		else
			return 0;
	}
	
	Uint32 TorrentFileStream::Private::lastChunk()
	{
		if (tc->getStats().multi_file_torrent)
			return tc->getTorrentFile(file_index).getLastChunk();
		else
			return tc->getStats().total_chunks - 1;
	}
	
	Uint32 TorrentFileStream::Private::lastChunkSize()
	{
		if (tc->getStats().multi_file_torrent)
			return tc->getTorrentFile(file_index).getLastChunkSize();
		else
			return cman->getChunk(lastChunk())->getSize();
	}
	
	qint64 TorrentFileStream::Private::readData(char* data, qint64 maxlen)
	{
		// First update so we now until how far we can go
		update();
		
		// Check if there is something to read
		if (current_byte_offset == current_limit)
			return 0;
		
		qint64 bytes_read = 0;
		while (bytes_read < maxlen && current_byte_offset < current_limit)
		{
			qint64 allowed = qMin((qint64)(current_limit - current_byte_offset),maxlen - bytes_read);
			qint64 ret = readCurrentChunk(data + bytes_read,allowed);
			bytes_read += ret;
		}
		
		timer.update();
		// Make sure we do not cache to much during streaming
		if (timer.getElapsed() > 10000)
			cman->checkMemoryUsage();
		return bytes_read;
	}
	
	qint64 TorrentFileStream::Private::readCurrentChunk(char* data, qint64 maxlen)
	{
		Chunk* c = cman->getChunk(current_chunk);
		// First make sure we have the chunk
		if (!current_chunk_data)
		{
			current_chunk_offset = 0;
			current_chunk_offset = current_chunk == firstChunk() ? firstChunkOffset() : 0;
			current_chunk_data = c->getPiece(0,c->getSize(),true);
		}
		
		// Calculate how much we can read
		qint64 allowed = c->getSize() - current_chunk_offset;
		if (allowed > maxlen)
			allowed = maxlen;
		
		// Copy data
		memcpy(data,current_chunk_data->data() + current_chunk_offset,allowed);
		// Update internal state
		current_byte_offset += allowed;
		current_chunk_offset += allowed;
		if (current_chunk_offset == c->getSize())
		{
			// The whole chunk was read, so go to the next chunk
			current_chunk++;
			current_chunk_data.reset();
			current_chunk_offset = 0;
		}
		
		return allowed;
	}




}
