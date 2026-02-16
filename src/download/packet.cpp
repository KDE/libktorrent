/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "packet.h"
#include "request.h"
#include <QString>
#include <cstring>
#include <diskio/chunk.h>
#include <net/socketdevice.h>
#include <peer/peer.h>
#include <util/bitset.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
Packet::Packet(Uint32 size, Uint8 type) noexcept
    : size(size)
    , written(0)
    , type(type)
{
    Uint8 *data = getData();
    WriteUint32(data, 0, size - 4);
    data[4] = type;
}

Packet::Ptr Packet::create(Uint8 type)
{
    constexpr Uint32 size = 5;
    return Ptr(new (size) Packet(size, type));
}

Packet::Ptr Packet::create(Uint16 port)
{
    constexpr Uint32 size = 7;
    Ptr pkt(new (size) Packet(size, PORT));
    WriteUint16(pkt->getData(), 5, port);
    return pkt;
}

Packet::Ptr Packet::create(Uint32 chunk, Uint8 type)
{
    constexpr Uint32 size = 9;
    Ptr pkt(new (size) Packet(size, type));
    WriteUint32(pkt->getData(), 5, chunk);
    return pkt;
}

Packet::Ptr Packet::create(const BitSet &bs)
{
    const Uint32 size = 5 + bs.getNumBytes();
    Ptr pkt(new (size) Packet(size, BITFIELD));
    memcpy(pkt->getData() + 5, bs.getData(), bs.getNumBytes());
    return pkt;
}

Packet::Ptr Packet::create(const Request &r, Uint8 type)
{
    constexpr Uint32 size = 17;
    Ptr pkt(new (size) Packet(size, type));
    Uint8 *data = pkt->getData();
    WriteUint32(data, 5, r.getIndex());
    WriteUint32(data, 9, r.getOffset());
    WriteUint32(data, 13, r.getLength());
    return pkt;
}

Packet::Ptr Packet::create(Uint32 index, Uint32 begin, Uint32 len, Chunk *ch)
{
    const Uint32 size = 13 + len;
    Ptr pkt(new (size) Packet(size, PIECE));
    Uint8 *data = pkt->getData();
    WriteUint32(data, 5, index);
    WriteUint32(data, 9, begin);
    ch->readPiece(begin, len, data + 13);
    return pkt;
}

Packet::Ptr Packet::create(Uint8 ext_id, const QByteArray &ext_data)
{
    const Uint32 size = 6 + ext_data.size();
    Ptr pkt(new (size) Packet(size, EXTENDED));
    Uint8 *data = pkt->getData();
    data[5] = ext_id;
    memcpy(data + 6, ext_data.data(), ext_data.size());
    return pkt;
}

bool Packet::isPiece(const Request &req) const
{
    const Uint8 *data = getData();
    return (data[4] == PIECE) && (ReadUint32(data, 5) == req.getIndex()) && (ReadUint32(data, 9) == req.getOffset()) && (size - 13 == req.getLength());
}

Packet::Ptr Packet::makeRejectOfPiece() const
{
    if (getType() != PIECE) {
        return Ptr();
    }

    const Uint8 *data = getData();
    Uint32 idx = bt::ReadUint32(data, 5);
    Uint32 off = bt::ReadUint32(data, 9);
    Uint32 len = size - 13;

    //  Out(SYS_CON|LOG_DEBUG) << "Packet::makeRejectOfPiece " << idx << " " << off << " " << len << endl;
    return create(Request(idx, off, len, nullptr), bt::REJECT_REQUEST);
}

/*
QString Packet::debugString() const
{
    if (!data)
        return QString::null;

    switch (data[4])
    {
        case CHOKE : return QString("CHOKE %1 %2").arg(hdr_length).arg(data_length);
        case UNCHOKE : return QString("UNCHOKE %1 %2").arg(hdr_length).arg(data_length);
        case INTERESTED : return QString("INTERESTED %1 %2").arg(hdr_length).arg(data_length);
        case NOT_INTERESTED : return QString("NOT_INTERESTED %1 %2").arg(hdr_length).arg(data_length);
        case HAVE : return QString("HAVE %1 %2").arg(hdr_length).arg(data_length);
        case BITFIELD : return QString("BITFIELD %1 %2").arg(hdr_length).arg(data_length);
        case PIECE : return QString("PIECE %1 %2").arg(hdr_length).arg(data_length);
        case REQUEST : return QString("REQUEST %1 %2").arg(hdr_length).arg(data_length);
        case CANCEL : return QString("CANCEL %1 %2").arg(hdr_length).arg(data_length);
        default: return QString("UNKNOWN %1 %2").arg(hdr_length).arg(data_length);
    }
}
*/

int Packet::send(net::SocketDevice *sock, Uint32 max_to_send)
{
    Uint32 bw = size - written;
    if (!bw) { // nothing to write
        return 0;
    }

    if (bw > max_to_send && max_to_send > 0) {
        bw = max_to_send;
    }
    int ret = sock->send(getData() + written, bw);
    if (ret > 0) {
        written += ret;
    }
    return ret;
}

}
