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
    : data(cap)
    , start(0)
    , buf_size(0)
{
}

bt::Uint32 CircularBuffer::read(QSpan<std::byte> buf)
{
    if (empty()) {
        return 0;
    }

    const bt::Uint32 to_read = buf_size < buf.size() ? buf_size : buf.size();

    const auto r1 = firstRange();
    if (r1.size() >= to_read) {
        memcpy(buf.data(), r1.data(), to_read);
    } else { // s < to_read
        memcpy(buf.data(), r1.data(), r1.size());
        const auto r2 = secondRange();
        memcpy(buf.data() + r1.size(), r2.data(), to_read - r1.size());
    }

    start = (start + to_read) % capacity();
    buf_size -= to_read;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::read 1 " << size() << " " << capacity() << endl;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::read 2 " << start << " " << to_read << endl;
    return to_read;
}

bt::Uint32 CircularBuffer::write(QByteArrayView buf)
{
    if (full()) {
        return 0;
    }

    const bt::Uint32 free_space = capacity() - buf_size;
    const bt::Uint32 to_write = free_space < buf.size() ? free_space : buf.size();

    const bt::Uint32 write_pos = (start + buf_size) % capacity();
    if (write_pos + to_write > capacity()) {
        const bt::Uint32 w = (capacity() - write_pos);
        memcpy(data.data() + write_pos, buf.data(), w);
        memcpy(data.data(), buf.data() + w, to_write - w);
    } else {
        memcpy(data.data() + write_pos, buf.data(), to_write);
    }

    buf_size += to_write;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::write 1 " << size() << " " << capacity() << endl;
    // Out(SYS_GEN|LOG_DEBUG) << "CircularBuffer::write 2 " << start << " " << to_write << endl;
    return to_write;
}

QByteArrayView CircularBuffer::firstRange()
{
    if (start + buf_size > capacity()) {
        return QByteArrayView{data}.sliced(start);
    } else {
        return QByteArrayView{data}.sliced(start, buf_size);
    }
}

QByteArrayView CircularBuffer::secondRange()
{
    if (start + buf_size > capacity()) {
        return QByteArrayView{data}.first(buf_size - (capacity() - start));
    } else {
        return QByteArrayView{};
    }
}

}
