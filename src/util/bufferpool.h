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

#ifndef BUFFERPOOL_H_
#define BUFFERPOOL_H_

#include <map>
#include <list>
#include <boost/shared_array.hpp>
#include <QMutex>
#include <QWeakPointer>
#include <QSharedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
	class BufferPool;

	/**
	 * Buffer object, extends boost shared_array with a size and capacity property.
	 **/
	class KTORRENT_EXPORT Buffer
	{
	public:
		typedef QSharedPointer<Buffer> Ptr;
		typedef boost::shared_array<bt::Uint8> Data;

		Buffer(Data data, bt::Uint32 fill, bt::Uint32 cap, QWeakPointer<BufferPool> pool);
		virtual ~Buffer();

		/// Get the buffers capacity
		bt::Uint32 capacity() const {return cap;}

		/// Get the current size
		bt::Uint32 size() const {return fill;}

		/// Set the current size
		void setSize(bt::Uint32 s) {fill = s;}

		/// Get a pointer to the data
		bt::Uint8* get() {return data.get();}

	private:
		Data data;
		bt::Uint32 fill;
		bt::Uint32 cap;
		QWeakPointer<BufferPool> pool;
	};

	/**
	 * Keeps track of a pool of buffers.
	 **/
	class KTORRENT_EXPORT BufferPool
	{
	public:
		BufferPool();
		virtual ~BufferPool();

		/**
		 * Set the weak pointer to the buffer pool itself.
		 * @param wp The weak pointer
		 * */
		void setWeakPointer(QWeakPointer<BufferPool> wp) {self = wp;}

		/**
		 * Get a buffer for a given size.
		 * The buffer returned might be bigger then the requested size.
		 * @param min_size The minimum size it should be
		 * @return A new Buffer
		 **/
		Buffer::Ptr get(bt::Uint32 min_size);

		/**
		 * Release a buffer, puts it into the free list.
		 * @param data The Buffer::Data
		 * @param size The size of the data object
		 **/
		void release(Buffer::Data data, bt::Uint32 size);

		/**
		 * Clear the pool.
		 **/
		void clear();

		typedef QSharedPointer<BufferPool> Ptr;

	private:
		typedef std::map<bt::Uint32, std::list<Buffer::Data> > FreeBufferMap;
		QMutex mutex;
		FreeBufferMap free_buffers;
		QWeakPointer<BufferPool> self;
	};
} /* namespace bt */

#endif /* BUFFERPOOL_H_ */
