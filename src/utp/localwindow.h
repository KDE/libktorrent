/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_LOCALWINDOW_H
#define UTP_LOCALWINDOW_H

#include "packetbuffer.h"
#include <QSharedPointer>
#include <ktorrent_export.h>
#include <util/constants.h>
#include <vector>

namespace utp
{
struct SelectiveAck;
struct Header;

const bt::Uint32 DEFAULT_CAPACITY = 64 * 1024;

struct WindowPacket {
    WindowPacket(bt::Uint16 seq_nr);
    WindowPacket(bt::Uint16 seq_nr, bt::Buffer::Ptr packet, bt::Uint32 data_off);
    ~WindowPacket();

    bt::Uint32 read(bt::Uint8 *dst, bt::Uint32 max_len);
    bool fullyRead() const;
    void set(bt::Buffer::Ptr packet, bt::Uint32 data_off);

    bt::Uint16 seq_nr;
    bt::Buffer::Ptr packet;
    bt::Uint32 bytes_read;
};

/*!
    Manages the local window of a UTP connection.
*/
class KTORRENT_EXPORT LocalWindow
{
public:
    LocalWindow(bt::Uint32 cap = DEFAULT_CAPACITY);
    virtual ~LocalWindow();

    //! Get back the available space
    bt::Uint32 availableSpace() const
    {
        return window_space;
    }

    //! Get back how large the window is
    bt::Uint32 currentWindow() const
    {
        return capacity - window_space;
    }

    //! Get the window capacity
    bt::Uint32 windowCapacity() const
    {
        return capacity;
    }

    //! A packet was received
    bool packetReceived(const Header *hdr, bt::Buffer::Ptr packet, bt::Uint32 data_off);

    //! Set the last sequence number
    void setLastSeqNr(bt::Uint16 lsn);

    //! Get the last sequence number we can safely ack
    bt::Uint16 lastSeqNr() const
    {
        return last_seq_nr;
    }

    //! Is the window empty
    bool isEmpty() const
    {
        return incoming_packets.empty();
    }

    //! Read from the local window
    bt::Uint32 read(bt::Uint8 *data, bt::Uint32 max_len);

    //! Is there something to read ?
    bool isReadable() const
    {
        return bytes_available > 0;
    }

    //! The amount of bytes available
    bt::Uint32 bytesAvailable() const
    {
        return bytes_available;
    }

    //! Get the number of selective ack bits needed when sending a packet
    bt::Uint32 selectiveAckBits() const;

    //! Fill a SelectiveAck structure
    void fillSelectiveAck(SelectiveAck *sack);

private:
    typedef std::vector<WindowPacket> WindowPacketList;

    bt::Uint16 last_seq_nr;
    bt::Uint16 first_seq_nr;
    bt::Uint32 capacity;
    WindowPacketList incoming_packets;
    bt::Uint32 window_space;
    bt::Uint32 bytes_available;
};

}

#endif // UTP_LOCALWINDOW_H
