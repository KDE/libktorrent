/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPACKETREADER_H
#define BTPACKETREADER_H

#include <cstddef>
#include <deque>
#include <optional>

#include <QMutex>

#include <ktorrent_export.h>
#include <net/trafficshapedsocket.h>
#include <util/array.h>
#include <util/constants.h>

namespace bt
{
class PeerInterface;

/*!
 * \headerfile peer/packetreader.h
 * \brief A partially or fully received BitTorrent packet that is yet to be processed.
 */
struct IncomingPacket {
    Array<Uint8> data;
    Uint32 read = 0;

    explicit IncomingPacket(Uint32 size) noexcept;

    IncomingPacket(IncomingPacket &&other) = default;
    IncomingPacket &operator=(IncomingPacket &&other) = default;

    IncomingPacket(const IncomingPacket &) = delete;
    IncomingPacket &operator=(const IncomingPacket &) = delete;
};

/*!
 * \headerfile peer/packetreader.h
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
    std::optional<IncomingPacket> dequeuePacket();

private:
    bool error;
    std::deque<IncomingPacket> packet_queue;
    QMutex mutex;
    Uint8 len[4];
    int len_received;
    Uint32 max_packet_size;
};

}

#endif
