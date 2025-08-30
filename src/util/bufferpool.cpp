/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "bufferpool.h"

namespace bt
{
Buffer::Buffer(Data data, bt::Uint32 fill, QWeakPointer<BufferPool> pool)
    : data(std::move(data))
    , fill(fill)
    , pool(pool)
{
}

Buffer::~Buffer()
{
    QSharedPointer<BufferPool> ptr = pool.toStrongRef();
    if (ptr)
        ptr->release(std::move(data));
}

BufferPool::BufferPool()
{
}

BufferPool::~BufferPool()
{
}

Buffer::Ptr BufferPool::get(bt::Uint32 min_size)
{
    QMutexLocker lock(&mutex);
    FreeBufferMap::iterator i = free_buffers.lower_bound(min_size);
    if (i != free_buffers.end() && !i->second.empty()) {
        Buffer::Data data = std::move(i->second.front());
        i->second.pop_front();
        return Buffer::Ptr(new Buffer(std::move(data), min_size, self));
    } else {
        return Buffer::Ptr(new Buffer(Buffer::Data(min_size), min_size, self));
    }
}

void BufferPool::release(Buffer::Data data)
{
    QMutexLocker lock(&mutex);
    free_buffers[data.size()].push_back(std::move(data));
}

void BufferPool::clear()
{
    QMutexLocker lock(&mutex);
    free_buffers.clear();
}

} /* namespace bt */
