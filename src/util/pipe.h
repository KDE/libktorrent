/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_PIPE_H
#define BT_PIPE_H

#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
/**
    Cross platform pipe implementation, uses socketpair on unix and a TCP connection over the localhost in windows.
*/
class KTORRENT_EXPORT Pipe
{
public:
    Pipe();
    virtual ~Pipe();

    /// Get the reader socket
    int readerSocket() const
    {
        return reader;
    }

    /// Get the writer socket
    int writerSocket() const
    {
        return writer;
    }

    /// Write data to the write end of the pipe
    int write(const Uint8 *data, int len);

    /// Read data from the read end of the pipe
    int read(Uint8 *buffer, int max_len);

protected:
    int reader;
    int writer;
};

}

#endif // BT_PIPE_H
