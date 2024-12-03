/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPACKETREADER_H
#define BTPACKETREADER_H

#include <QList>
#include <QMutex>
#include <deque>
#include <ktorrent_export.h>
#include <net/trafficshapedsocket.h>

namespace bt
{
class PeerInterface;

struct IncomingPacket {
    QScopedArrayPointer<Uint8> data;
    Uint32 size;
    Uint32 read;

    IncomingPacket(Uint32 size);

    typedef QSharedPointer<IncomingPacket> Ptr;
};

/**
 * Chops up the raw byte stream from a socket into bittorrent packets
 * @author Joris Guisson
 */
class KTORRENT_EXPORT PacketReader : public net::SocketReader
{
public:
    PacketReader(Uint32 max_packet_size);
    ~PacketReader() override;

    /**
     * Push packets to Peer (runs in main thread)
     * @param peer The PeerInterface which will handle the packet
     */
    void update(PeerInterface &peer);

    /// Did an error occur
    bool ok() const
    {
        return !error;
    }

    void onDataReady(QByteArrayView data) override;

private:
    Uint32 newPacket(QByteArrayView data);
    Uint32 readPacket(QByteArrayView data);
    IncomingPacket::Ptr dequeuePacket();

private:
    bool error;
#ifndef DO_NOT_USE_DEQUE
    std::deque<IncomingPacket::Ptr> packet_queue;
#else
    QList<IncomingPacket::Ptr> packet_queue;
#endif
    QMutex mutex;
    Uint8 len[4];
    int len_received;
    Uint32 max_packet_size;
};

}

#endif
