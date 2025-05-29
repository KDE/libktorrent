/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BUFFERPOOL_H_
#define BUFFERPOOL_H_

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
 * Buffer object, extends shared_ptr with a size and capacity property.
 **/
class KTORRENT_EXPORT Buffer
{
public:
    typedef QSharedPointer<Buffer> Ptr;
    typedef std::shared_ptr<bt::Uint8[]> Data;

    Buffer(Data data, bt::Uint32 fill, bt::Uint32 cap, QWeakPointer<BufferPool> pool);
    virtual ~Buffer();

    //! Get the buffers capacity
    bt::Uint32 capacity() const
    {
        return cap;
    }

    //! Get the current size
    bt::Uint32 size() const
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
        return data.get();
    }

private:
    Data data;
    bt::Uint32 fill;
    bt::Uint32 cap;
    QWeakPointer<BufferPool> pool;
};

/*!
 * Keeps track of a pool of buffers.
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
     * \param size The size of the data object
     **/
    void release(Buffer::Data data, bt::Uint32 size);

    /*!
     * Clear the pool.
     **/
    void clear();

    typedef QSharedPointer<BufferPool> Ptr;

private:
    typedef std::map<bt::Uint32, std::list<Buffer::Data>> FreeBufferMap;
    QMutex mutex;
    FreeBufferMap free_buffers;
    QWeakPointer<BufferPool> self;
};
} /* namespace bt */

#endif /* BUFFERPOOL_H_ */
