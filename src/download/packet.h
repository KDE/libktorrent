/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPACKET_H
#define BTPACKET_H

#include <QSharedPointer>
#include <util/constants.h>

namespace net
{
class SocketDevice;
}

namespace bt
{
class BitSet;
class Request;
class Chunk;
class Peer;

/**
 * @author Joris Guisson
 *
 * Packet off data, which gets sent to a Peer
 */
class Packet
{
public:
    Packet(Uint8 type);
    Packet(Uint16 port);
    Packet(Uint32 chunk, Uint8 type);
    Packet(const BitSet &bs);
    Packet(const Request &req, Uint8 type);
    Packet(Uint32 index, Uint32 begin, Uint32 len, Chunk *ch);
    Packet(Uint8 ext_id, const QByteArray &ext_data); // extension protocol packet
    ~Packet();

    /// Get the packet type
    Uint8 getType() const
    {
        return type;
    }

    bool isOK() const;

    const Uint8 *getData() const
    {
        return data;
    }
    Uint8 *getData()
    {
        return data;
    }
    Uint32 getDataLength() const
    {
        return size;
    }

    /// Is the packet sent ?
    Uint32 isSent() const
    {
        return written == size;
    }

    /**
     * If this packet is a piece, make a reject for it
     * @return The newly created Packet, 0 if this is not a piece
     */
    Packet *makeRejectOfPiece();

    /// Are we sending this packet ?
    bool sending() const
    {
        return written > 0;
    }

    /**
     * Is this a piece packet which matches a request
     * @param req The request
     * @return If this is a piece in response of this request
     */
    bool isPiece(const Request &req) const;

    /**
     * Send the packet over a SocketDevice
     * @param sock The socket
     * @param max_to_send Max bytes to send
     * @return int Return value of send call from SocketDevice
     **/
    int send(net::SocketDevice *sock, Uint32 max_to_send);

    typedef QSharedPointer<Packet> Ptr;

private:
    Uint8 *data;
    Uint32 size;
    Uint32 written;
    Uint8 type;
};

}

#endif
