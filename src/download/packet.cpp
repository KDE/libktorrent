/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "packet.h"
#include "request.h"
#include <diskio/chunk.h>
#include <net/socketdevice.h>
#include <peer/peer.h>
#include <qstring.h>
#include <string.h>
#include <util/bitset.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
static Uint8 *AllocPacket(Uint32 size, Uint8 type)
{
    Uint8 *data = new Uint8[size];
    WriteUint32(data, 0, size - 4);
    data[4] = type;
    return data;
}

Packet::Packet(Uint8 type)
    : data(nullptr)
    , size(5)
    , written(0)
    , type(type)
{
    data = AllocPacket(size, type);
}

Packet::Packet(Uint16 port)
    : data(nullptr)
    , size(7)
    , written(0)
    , type(PORT)
{
    data = AllocPacket(size, PORT);
    WriteUint16(data, 5, port);
}

Packet::Packet(Uint32 chunk, Uint8 type)
    : data(nullptr)
    , size(9)
    , written(0)
    , type(type)
{
    data = AllocPacket(size, type);
    WriteUint32(data, 5, chunk);
}

Packet::Packet(const BitSet &bs)
    : data(nullptr)
    , size(5 + bs.getNumBytes())
    , written(0)
    , type(BITFIELD)
{
    data = AllocPacket(size, BITFIELD);
    memcpy(data + 5, bs.getData(), bs.getNumBytes());
}

Packet::Packet(const Request &r, Uint8 type)
    : data(nullptr)
    , size(17)
    , written(0)
    , type(type)
{
    data = AllocPacket(size, type);
    WriteUint32(data, 5, r.getIndex());
    WriteUint32(data, 9, r.getOffset());
    WriteUint32(data, 13, r.getLength());
}

Packet::Packet(Uint32 index, Uint32 begin, Uint32 len, Chunk *ch)
    : data(nullptr)
    , size(13 + len)
    , written(0)
    , type(PIECE)
{
    data = AllocPacket(size, PIECE);
    WriteUint32(data, 5, index);
    WriteUint32(data, 9, begin);
    ch->readPiece(begin, len, data + 13);
}

Packet::Packet(Uint8 ext_id, const QByteArray &ext_data)
    : data(nullptr)
    , size(6 + ext_data.size())
    , written(0)
    , type(EXTENDED)
{
    data = AllocPacket(size, EXTENDED);
    data[5] = ext_id;
    memcpy(data + 6, ext_data.data(), ext_data.size());
}

Packet::~Packet()
{
    delete[] data;
}

bool Packet::isPiece(const Request &req) const
{
    return (data[4] == PIECE) && (ReadUint32(data, 5) == req.getIndex()) && (ReadUint32(data, 9) == req.getOffset()) && (size - 13 == req.getLength());
}

Packet *Packet::makeRejectOfPiece()
{
    if (getType() != PIECE)
        return nullptr;

    Uint32 idx = bt::ReadUint32(data, 5);
    Uint32 off = bt::ReadUint32(data, 9);
    Uint32 len = size - 13;

    //  Out(SYS_CON|LOG_DEBUG) << "Packet::makeRejectOfPiece " << idx << " " << off << " " << len << endl;
    return new Packet(Request(idx, off, len, nullptr), bt::REJECT_REQUEST);
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
bool Packet::isOK() const
{
    return bool(data);
}

int Packet::send(net::SocketDevice *sock, Uint32 max_to_send)
{
    Uint32 bw = size - written;
    if (!bw) // nothing to write
        return 0;

    if (bw > max_to_send && max_to_send > 0)
        bw = max_to_send;
    int ret = sock->send(data + written, bw);
    if (ret > 0)
        written += ret;
    return ret;
}

}
