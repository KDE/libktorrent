/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_PIPE_H
#define BT_PIPE_H

#include <cstddef>

#include <QByteArrayView>
#include <QSpan>

#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
/*!
 * \headerfile util/pipe.h
 * \brief Cross platform pipe implementation.
 *
 * Uses socketpair on unix and a TCP connection over the localhost in windows.
 */
class KTORRENT_EXPORT Pipe
{
public:
    Pipe();
    ~Pipe();

    //! Get the reader socket
    [[nodiscard]] int readerSocket() const
    {
        return reader;
    }

    //! Get the writer socket
    [[nodiscard]] int writerSocket() const
    {
        return writer;
    }

    //! Write data to the write end of the pipe
    int write(QByteArrayView data);

    //! Read data from the read end of the pipe
    int read(QSpan<std::byte> buffer);

protected:
    int reader;
    int writer;
};

}

#endif // BT_PIPE_H
