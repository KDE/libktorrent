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
#include <download/streamingchunkselector.h>
#include "torrentcontrol.h"

namespace bt
{
	
	
	class TorrentFileStream::Private
	{
	public:
		Private(TorrentControl* tc,ChunkManager* cman,TorrentFileStream* p);
		Private(TorrentControl* tc,Uint32 file_index,ChunkManager* cman,TorrentFileStream* p);
		~Private();
		
		void reset();
		void update();
		Uint32 lastChunk();
		Uint32 firstChunk();
		Uint32 firstChunkOffset();
		Uint32 lastChunkSize();
		qint64 readData(char* data, qint64 maxlen);
		qint64 readCurrentChunk(char* data, qint64 maxlen);
		bool seek(qint64 pos);
		
	public:
		TorrentControl* tc;
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
		StreamingChunkSelector* csel;
	};
	
	
	TorrentFileStream::TorrentFileStream(TorrentControl* tc, ChunkManager* cman,QObject* parent)
		: QIODevice(parent),d(new Private(tc,cman,this))
	{
	}
	
	TorrentFileStream::TorrentFileStream(TorrentControl* tc, Uint32 file_index, ChunkManager* cman,QObject* parent)
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
		d->update();
		return d->seek(pos);
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
	
	QString TorrentFileStream::path() const
	{
		if (d->tc->getStats().multi_file_torrent)
		{
			return d->tc->getTorrentFile(d->file_index).getPathOnDisk();
		}
		else
		{
			return d->tc->getStats().output_path;
		}
	}
	
	void TorrentFileStream::chunkDownloaded(TorrentInterface* tc, Uint32 chunk)
	{
		Q_UNUSED(tc);
		if (d->current_byte_offset == d->current_limit && chunk == d->current_chunk)
		{
			d->update();
			emit readyRead();
		}
	}
	
	void TorrentFileStream::emitReadChannelFinished()
	{
		emit readChannelFinished();
	}

	
	//////////////////////////////////////////////////
	TorrentFileStream::Private::Private(TorrentControl* tc, ChunkManager* cman, TorrentFileStream* p) 
		: tc(tc),file_index(0),cman(cman),p(p),
		current_byte_offset(0),current_limit(0),opened(false),
		current_chunk_offset(0),csel(0)
	{
		current_chunk = firstChunk();
		connect(tc,SIGNAL(chunkDownloaded(bt::TorrentInterface*,bt::Uint32)),
				p,SLOT(chunkDownloaded(bt::TorrentInterface*,bt::Uint32)));
		
		csel = new StreamingChunkSelector();
		tc->setChunkSelector(csel);
		csel->setSequentialRange(firstChunk(),lastChunk());
	}
	
	TorrentFileStream::Private::Private(TorrentControl* tc, 
										Uint32 file_index, 
										ChunkManager* cman,
										TorrentFileStream* p)
		: tc(tc),file_index(file_index),cman(cman),p(p),
		current_byte_offset(0),current_limit(0),opened(false),
		current_chunk_offset(0),csel(0)
	{
		current_chunk = firstChunk();
		current_chunk_offset = firstChunkOffset();
		
		csel = new StreamingChunkSelector();
		tc->setChunkSelector(csel); 
		csel->setSequentialRange(firstChunk(),lastChunk());
	}
	
	TorrentFileStream::Private::~Private()
	{
		tc->setChunkSelector(0); // Force creation of new chunk selector
	}
	
	void TorrentFileStream::Private::reset()
	{
		current_byte_offset = 0;
		current_limit = 0;
		current_chunk = firstChunk();
		current_chunk_offset = firstChunkOffset();
		current_chunk_data.reset();
		cman->checkMemoryUsage();
	}
	
	void TorrentFileStream::Private::update()
	{
		if (current_limit == (Uint64)p->size())
			return;
		
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
			
			if (current_limit == (Uint64)p->size())
				p->emitReadChannelFinished();
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
		
		if (current_limit == (Uint64)p->size())
			p->emitReadChannelFinished();
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
	
	bool TorrentFileStream::Private::seek(qint64 pos)
	{
		if (pos >= (qint64)current_limit || pos < 0)
			return false;
		
		current_byte_offset = pos;
		current_chunk_offset = 0;
		current_chunk_data.reset();
		if (tc->getStats().multi_file_torrent)
		{
			bt::Uint64 tor_byte_offset = firstChunk() * tc->getStats().chunk_size;
			tor_byte_offset += firstChunkOffset() + pos;
			current_chunk = tor_byte_offset / tc->getStats().chunk_size;
			current_chunk_offset = tor_byte_offset % tc->getStats().chunk_size;
		}
		else
		{
			current_chunk = current_byte_offset / tc->getStats().chunk_size;
			current_chunk_offset = current_byte_offset % tc->getStats().chunk_size;
		}
		
		csel->setCursor(current_chunk);
		return true;
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
		
		// Make sure we do not cache to much during streaming
		if (timer.getElapsedSinceUpdate() > 10000)
		{
			cman->checkMemoryUsage();
			timer.update();
		}
		
		return bytes_read;
	}
	
	qint64 TorrentFileStream::Private::readCurrentChunk(char* data, qint64 maxlen)
	{
		//Out(SYS_GEN|LOG_DEBUG) << "readCurrentChunk s " << current_chunk << " " << current_chunk_offset << endl;
		Chunk* c = cman->getChunk(current_chunk);
		// First make sure we have the chunk
		if (!current_chunk_data)
			current_chunk_data = c->getPiece(0,c->getSize(),true);
		
		// Calculate how much we can read
		qint64 allowed = c->getSize() - current_chunk_offset;
		if (allowed > maxlen)
			allowed = maxlen;
		
		//Out(SYS_GEN|LOG_DEBUG) << "readCurrentChunk r " << allowed << endl;
		
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
			csel->setCursor(current_chunk);
		}
		
		//Out(SYS_GEN|LOG_DEBUG) << "readCurrentChunk f " << current_chunk << " " << current_chunk_offset << endl;
		return allowed;
	}

}
