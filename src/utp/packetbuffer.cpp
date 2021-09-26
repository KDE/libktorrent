/*
    SPDX-FileCopyrightText: 2011 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "packetbuffer.h"
#include "utpprotocol.h"
#include <QMutex>
#include <QMutexLocker>
#include <list>

namespace utp
{
bt::BufferPool::Ptr PacketBuffer::pool;

PacketBuffer::PacketBuffer()
    : header(nullptr)
    , extension(nullptr)
    , payload(nullptr)
    , size(0)
{
    if (!pool) {
        pool = bt::BufferPool::Ptr(new bt::BufferPool());
        pool->setWeakPointer(pool.toWeakRef());
    }
    buffer = pool->get(MAX_SIZE);
}

PacketBuffer::~PacketBuffer()
{
}

void PacketBuffer::clearPool()
{
    pool.clear();
}

bool PacketBuffer::setHeader(const Header &hdr, bt::Uint32 extension_length)
{
    if (Header::size() - extension_length > headRoom())
        return false;

    if (payload)
        header = payload - Header::size() - extension_length;
    else
        header = buffer->get();

    hdr.write(header);
    extension = header + Header::size();
    if (payload)
        size = (buffer->get() + MAX_SIZE) - header;
    else
        size = Header::size() + extension_length;

    return true;
}

bt::Uint32 PacketBuffer::fillData(bt::CircularBuffer &cbuf, bt::Uint32 to_read)
{
    // Make sure we leave enough room for a header
    if (to_read > MAX_SIZE - Header::size())
        to_read = MAX_SIZE - Header::size();

    // Data is put at the end of the buffer, so we can put headers easily in front of it
    payload = (buffer->get() + MAX_SIZE) - to_read;
    cbuf.read(payload, to_read);
    size = to_read;

    header = extension = payload;
    return to_read;
}

bt::Uint32 PacketBuffer::fillData(const bt::Uint8 *data, bt::Uint32 data_size)
{
    if (data_size > MAX_SIZE)
        data_size = MAX_SIZE;

    payload = (buffer->get() + MAX_SIZE) - data_size;
    memcpy(payload, data, data_size);
    header = extension = payload;
    size = data_size;
    return data_size;
}

void PacketBuffer::fillDummyData(bt::Uint32 amount)
{
    header = extension = payload = (buffer->get() + MAX_SIZE) - amount;
    size += amount;
}

} /* namespace utp */
