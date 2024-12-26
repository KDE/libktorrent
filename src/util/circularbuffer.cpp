/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "circularbuffer.h"
#include "log.h"
#include <cstring>
#include <util/log.h>

namespace bt
{
CircularBuffer::CircularBuffer(Uint32 cap)
    : data(nullptr)
    , buf_capacity(cap)
    , start(0)
    , buf_size(0)
{
    data = new Uint8[cap];
}

CircularBuffer::~CircularBuffer()
{
    delete[] data;
}

bt::Uint32 CircularBuffer::read(bt::Uint8 *ptr, bt::Uint32 max_len)
{
    if (empty())
        return 0;

    bt::Uint32 to_read = buf_size < max_len ? buf_size : max_len;

    Range r = firstRange();
    bt::Uint32 s = r.second;
    if (s >= to_read) {
        memcpy(ptr, r.first, to_read);
    } else { // s < to_read
        memcpy(ptr, r.first, s);
        r = secondRange();
        memcpy(ptr + s, r.first, to_read - s);
    }

    start = (start + to_read) % buf_capacity;
    buf_size -= to_read;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::read 1 " << size() << " " << capacity() << endl;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::read 2 " << start << " " << to_read << endl;
    return to_read;
}

bt::Uint32 CircularBuffer::write(const bt::Uint8 *ptr, bt::Uint32 len)
{
    if (full())
        return 0;

    bt::Uint32 free_space = buf_capacity - buf_size;
    bt::Uint32 to_write = free_space < len ? free_space : len;

    bt::Uint32 write_pos = (start + buf_size) % buf_capacity;
    if (write_pos + to_write > buf_capacity) {
        bt::Uint32 w = (buf_capacity - write_pos);
        memcpy(data + write_pos, ptr, w);
        memcpy(data, ptr + w, to_write - w);
    } else {
        memcpy(data + write_pos, ptr, to_write);
    }

    buf_size += to_write;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::write 1 " << size() << " " << capacity() << endl;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::write 2 " << start << " " << to_write << endl;
    return to_write;
}

CircularBuffer::Range CircularBuffer::firstRange()
{
    if (start + buf_size > buf_capacity)
        return Range(data + start, buf_capacity - start);
    else
        return Range(data + start, buf_size);
}

CircularBuffer::Range CircularBuffer::secondRange()
{
    if (start + buf_size > buf_capacity)
        return Range(data, buf_size - (buf_capacity - start));
    else
        return Range((bt::Uint8 *)nullptr, 0);
}

}
