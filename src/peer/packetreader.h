/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPACKETREADER_H
#define BTPACKETREADER_H

#include <QMutex>
#include <boost/align/align_up.hpp>
#include <cstddef>
#include <deque>
#include <ktorrent_export.h>
#include <memory>
#include <net/trafficshapedsocket.h>
#include <new>

namespace bt
{
class PeerInterface;

/*!
 * \brief A partially or fully received BitTorrent packet that is yet to be processed.
 */
struct IncomingPacket {
    Uint32 size;
    Uint32 read;

    using Ptr = std::unique_ptr<IncomingPacket>;

private:
    static constexpr std::size_t data_alignment = 16;

    explicit IncomingPacket(Uint32 size) noexcept;

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
    IncomingPacket(const IncomingPacket &) = delete;
    IncomingPacket &operator=(const IncomingPacket &) = delete;

    //! Creates a packet of the given size
    static Ptr create(Uint32 size);

    //! Memory release function
    static void operator delete(void *p) noexcept
    {
        ::operator delete(p);
    }

    //! Returns a pointer to the packet data
    Uint8 *data() noexcept
    {
        return reinterpret_cast<Uint8 *>(this) + boost::alignment::align_up(sizeof(*this), data_alignment);
    }
};

/*!
 * \author Joris Guisson
 * \brief Chops up the raw byte stream from a socket into bittorrent packets.
 */
class KTORRENT_EXPORT PacketReader : public net::SocketReader
{
public:
    PacketReader(Uint32 max_packet_size);
    ~PacketReader() override;

    /*!
     * Push packets to Peer (runs in main thread)
     * \param peer The PeerInterface which will handle the packet
     */
    void update(PeerInterface &peer);

    //! Did an error occur
    [[nodiscard]] bool ok() const
    {
        return !error;
    }

    void onDataReady(Uint8 *buf, Uint32 size) override;

private:
    Uint32 newPacket(Uint8 *buf, Uint32 size);
    Uint32 readPacket(Uint8 *buf, Uint32 size);
    IncomingPacket::Ptr dequeuePacket();

private:
    bool error;
    std::deque<IncomingPacket::Ptr> packet_queue;
    QMutex mutex;
    Uint8 len[4];
    int len_received;
    Uint32 max_packet_size;
};

}

#endif
