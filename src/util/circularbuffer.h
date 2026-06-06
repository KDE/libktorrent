/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_CIRCULARBUFFER_H
#define BT_CIRCULARBUFFER_H

#include <utility>

#include <QByteArrayView>

#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
/*!
 * \headerfile util/circularbuffer.h
 * \brief Circular buffer for byte arrays.
 */
class KTORRENT_EXPORT CircularBuffer
{
public:
    CircularBuffer(bt::Uint32 cap = 64 * 1024);
    virtual ~CircularBuffer();

    /*!
        Read up to max_len bytes from the buffer and store it in data
        \param ptr The place to store the data
        \param max_len Maximum amount to read
        \return The amount read
    */
    virtual bt::Uint32 read(bt::Uint8 *ptr, bt::Uint32 max_len);

    /*!
        Write bytes from \c buf and store it in the window. Returns the number of bytes written.
    */
    virtual bt::Uint32 write(QByteArrayView buf);

    //! Is the buffer empty
    [[nodiscard]] bool empty() const
    {
        return buf_size == 0;
    }

    //! Is the buffer full
    [[nodiscard]] bool full() const
    {
        return buf_size == buf_capacity;
    }

    //! How much of the buffer is used
    [[nodiscard]] bt::Uint32 size() const
    {
        return buf_size;
    }

    //! How much capacity is available
    [[nodiscard]] bt::Uint32 capacity() const
    {
        return buf_capacity;
    }

    //! Get the available space
    [[nodiscard]] bt::Uint32 available() const
    {
        return buf_capacity - buf_size;
    }

private:
    using Range = std::pair<bt::Uint8 *, bt::Uint32>;

    //! Get the first range
    Range firstRange();
    Range secondRange();

private:
    bt::Uint8 *data;
    bt::Uint32 buf_capacity;
    bt::Uint32 start;
    bt::Uint32 buf_size;
};

}

#endif // BT_CIRCULARBUFFER_H
