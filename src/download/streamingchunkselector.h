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

#ifndef BT_STREAMINGCHUNKSELECTOR_H
#define BT_STREAMINGCHUNKSELECTOR_H

#include <ktorrent_export.h>
#include <download/chunkselector.h>


namespace bt 
{
	/**
		ChunkSelector which supports streaming mode.
		It has a range of chunks which are to be downloaded sequentially. And it has a cursor, to support jumping around
		in the stream.
	 */
	class KTORRENT_EXPORT StreamingChunkSelector : public bt::ChunkSelector
	{
	public:
		StreamingChunkSelector();
		virtual ~StreamingChunkSelector();
		
		virtual void init(ChunkManager* cman, Downloader* downer, PeerManager* pman);
		virtual bool select(bt::PieceDownloader* pd, bt::Uint32& chunk);
		virtual void dataChecked(const bt::BitSet& ok_chunks);
		virtual void reincluded(bt::Uint32 from, bt::Uint32 to);
		virtual void reinsert(bt::Uint32 chunk);
		virtual bool selectRange(bt::Uint32& from, bt::Uint32& to, bt::Uint32 max_len);
		
		/// Get the critical window size in chunks
		Uint32 criticialWindowSize() const {return critical_window_size;}
		
		/**
			Set the range to be downloaded sequentially.
			The cursor will be initialized to the first of the range.
			@param from Start of range
			@param to End of range
		 */
		void setSequentialRange(bt::Uint32 from, bt::Uint32 to);
		
		/// Set the cursor location
		void setCursor(bt::Uint32 chunk);
		
	private:
		void updateRange();
		void initRange();
		
	private:
		bt::Uint32 range_start;
		bt::Uint32 range_end;
		bt::Uint32 cursor;
		bt::Uint32 critical_window_size;
		std::list<Uint32> range;
	};

}

#endif // BT_STREAMINGCHUNKSELECTOR_H
