/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPACKET_H
#define BTPACKET_H

#include <cstddef>
#include <new>
#include <boost/align/align_up.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
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
 * \author Joris Guisson
 *
 * \brief Packet of data, which gets sent to a Peer.
 */
class Packet :
    public boost::intrusive_ref_counter<Packet>
{
public:
    using Ptr = boost::intrusive_ptr<Packet>;

private:
    static constexpr std::size_t data_alignment = 16;

    Packet(Uint32 size, Uint8 type) noexcept;

    //! Memory allocation function
    static void *operator new(std::size_t object_size, Uint32 data_size)
    {
        return ::operator new(boost::alignment::align_up(object_size, data_alignment) + data_size);
    }
    //! Placement delete function
    static void operator delete(void *p, Uint32) noexcept
    {
        operator delete(p);
    }

public:
    Packet(const Packet &) = delete;
    Packet &operator= (const Packet &) = delete;

    //! Memory release function
    static void operator delete(void *p) noexcept
    {
        ::operator delete(p);
    }

    static Ptr create(Uint8 type);
    static Ptr create(Uint16 port);
    static Ptr create(Uint32 chunk, Uint8 type);
    static Ptr create(const BitSet &bs);
    static Ptr create(const Request &req, Uint8 type);
    static Ptr create(Uint32 index, Uint32 begin, Uint32 len, Chunk *ch);
    static Ptr create(Uint8 ext_id, const QByteArray &ext_data); // extension protocol packet

    //! Get the packet type
    Uint8 getType() const
    {
        return type;
    }

    const Uint8 *getData() const
    {
        return reinterpret_cast<const Uint8 *>(this) + boost::alignment::align_up(sizeof(*this), data_alignment);
    }
    Uint8 *getData()
    {
        return reinterpret_cast<Uint8 *>(this) + boost::alignment::align_up(sizeof(*this), data_alignment);
    }
    Uint32 getDataLength() const
    {
        return size;
    }

    //! Is the packet sent ?
    Uint32 isSent() const
    {
        return written == size;
    }

    /*!
     * If this packet is a piece, make a reject for it
     * \return The newly created Packet, 0 if this is not a piece
     */
    Ptr makeRejectOfPiece() const;

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
    Uint32 size;
    Uint32 written;
    Uint8 type;
};

}

#endif
