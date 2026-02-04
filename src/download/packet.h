/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPACKET_H
#define BTPACKET_H

#include <cstddef>
#include <optional>

#include <ktorrent_export.h>
#include <util/array.h>
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

/*!
 * \headerfile download/packet.h
 * \author Joris Guisson
 *
 * \brief Packet of data, which gets sent to a Peer.
 */
class KTORRENT_EXPORT Packet
{
    Packet(Uint32 size, Uint8 type) noexcept;

public:
    Packet(Packet &&other) noexcept = default;
    Packet &operator=(Packet &&other) noexcept = default;

    Packet(const Packet &) = delete;
    Packet &operator=(const Packet &) = delete;

    static Packet create(Uint8 type);
    static Packet create(Uint16 port);
    static Packet create(Uint32 chunk, Uint8 type);
    static Packet create(const BitSet &bs);
    static Packet create(const Request &req, Uint8 type);
    static Packet create(Uint32 index, Uint32 begin, Uint32 len, Chunk *ch);
    static Packet create(Uint8 ext_id, const QByteArray &ext_data); // extension protocol packet

    //! Get the packet type
    Uint8 getType() const
    {
        return type;
    }

    const Uint8 *getData() const
    {
        return data.data();
    }
    Uint8 *getData()
    {
        return data.data();
    }
    Uint32 getDataLength() const
    {
        return data.size();
    }

    //! Is the packet sent ?
    Uint32 isSent() const
    {
        return written == data.size();
    }

    /*!
     * If this packet is a piece, make a reject for it
     * \return The newly created Packet, 0 if this is not a piece
     */
    std::optional<Packet> makeRejectOfPiece() const;

    //! Are we sending this packet ?
    bool sending() const
    {
        return written > 0;
    }

    /*!
     * Is this a piece packet which matches a request
     * \param req The request
     * \return If this is a piece in response of this request
     */
    bool isPiece(const Request &req) const;

    /*!
     * Send the packet over a SocketDevice
     * \param sock The socket
     * \param max_to_send Max bytes to send
     * \return int Return value of send call from SocketDevice
     **/
    int send(net::SocketDevice *sock, Uint32 max_to_send);

private:
    Array<Uint8> data;
    Uint8 type;
    Uint32 written = 0;
};
}

#endif
