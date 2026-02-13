/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BUFFERPOOL_H_
#define BUFFERPOOL_H_

#include "array.h"
#include <QMutex>
#include <QSharedPointer>
#include <QWeakPointer>
#include <ktorrent_export.h>
#include <list>
#include <map>
#include <util/constants.h>

namespace bt
{
class BufferPool;

/*!
 * \brief A fixed-capacity byte array that can be leased from a BufferPool.
 **/
class KTORRENT_EXPORT Buffer
{
public:
    using Ptr = QSharedPointer<Buffer>;
    // TODO(Qt6.8) use QByteArray with Qt::Initialization overload
    using Data = Array<bt::Uint8>;

    Buffer(Data data, bt::Uint32 fill, QWeakPointer<BufferPool> pool);
    virtual ~Buffer();

    //! Get the buffers capacity
    [[nodiscard]] bt::Uint32 capacity() const
    {
        return data.size();
    }

    //! Get the current size
    [[nodiscard]] bt::Uint32 size() const
    {
        return fill;
    }

    //! Set the current size
    void setSize(bt::Uint32 s)
    {
        fill = s;
    }

    //! Get a pointer to the data
    bt::Uint8 *get()
    {
        return data.data();
    }

private:
    Data data;
    bt::Uint32 fill;
    QWeakPointer<BufferPool> pool;
};

/*!
 * \brief Keeps track of a pool of buffers.
 **/
class KTORRENT_EXPORT BufferPool
{
public:
    BufferPool();
    virtual ~BufferPool();

    /*!
     * Set the weak pointer to the buffer pool itself.
     * \param wp The weak pointer
     * */
    void setWeakPointer(QWeakPointer<BufferPool> wp)
    {
        self = wp;
    }

    /*!
     * Get a buffer for a given size.
     * The buffer returned might be bigger then the requested size.
     * \param min_size The minimum size it should be
     * \return A new Buffer
     **/
    Buffer::Ptr get(bt::Uint32 min_size);

    /*!
     * Release a buffer, puts it into the free list.
     * \param data The Buffer::Data
     **/
    void release(Buffer::Data data);

    /*!
     * Clear the pool.
     **/
    void clear();

    using Ptr = QSharedPointer<BufferPool>;

private:
    using FreeBufferMap = std::map<bt::Uint32, std::list<Buffer::Data>>;
    QMutex mutex;
    FreeBufferMap free_buffers;
    QWeakPointer<BufferPool> self;
};
} /* namespace bt */

#endif /* BUFFERPOOL_H_ */
