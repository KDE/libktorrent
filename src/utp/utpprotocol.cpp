/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "utpprotocol.h"
#include <util/functions.h>

namespace utp
{
QString TypeToString(bt::Uint8 type)
{
    switch (type) {
    case ST_DATA:
        return QStringLiteral("DATA");
    case ST_FIN:
        return QStringLiteral("FIN");
    case ST_STATE:
        return QStringLiteral("STATE");
    case ST_RESET:
        return QStringLiteral("RESET");
    case ST_SYN:
        return QStringLiteral("SYN");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

void Header::read(const bt::Uint8 *data)
{
    type = (data[0] & 0xF0) >> 4;
    version = data[0] & 0x0F;
    extension = data[1];
    connection_id = bt::ReadUint16(data, 2);
    timestamp_microseconds = bt::ReadUint32(data, 4);
    timestamp_difference_microseconds = bt::ReadUint32(data, 8);
    wnd_size = bt::ReadUint32(data, 12);
    seq_nr = bt::ReadUint16(data, 16);
    ack_nr = bt::ReadUint16(data, 18);
}

void Header::write(bt::Uint8 *data) const
{
    data[0] = ((type << 4) & 0xF0) | (version & 0x0F);
    data[1] = extension;
    bt::WriteUint16(data, 2, connection_id);
    bt::WriteUint32(data, 4, timestamp_microseconds);
    bt::WriteUint32(data, 8, timestamp_difference_microseconds);
    bt::WriteUint32(data, 12, wnd_size);
    bt::WriteUint16(data, 16, seq_nr);
    bt::WriteUint16(data, 18, ack_nr);
}

bt::Uint32 Header::size()
{
    return 20;
}

PacketParser::PacketParser(const QByteArray &pkt)
    : packet((const bt::Uint8 *)pkt.data())
    , sack_found(false)
    , size(pkt.size())
    , data_off(0)
    , data_size(0)
{
    hdr.read(packet);
}

PacketParser::PacketParser(const bt::Uint8 *packet, bt::Uint32 size)
    : packet(packet)
    , sack_found(false)
    , size(size)
    , data_off(0)
    , data_size(0)
{
    hdr.read(packet);
}

PacketParser::~PacketParser()
{
}

bool PacketParser::parse()
{
    if (size < Header::size())
        return false;

    data_off = Header::size();

    // go over all header extensions to increase the data offset and watch out for selective acks
    int ext_id = hdr.extension;
    while (data_off < size && ext_id != 0) {
        const bt::Uint8 *ptr = packet + data_off;
        if (ext_id == SELECTIVE_ACK_ID) {
            sack_found = true;
            sack.extension = ptr[0];
            sack.length = ptr[1];
            if (data_off + 2 + sack.length > size)
                return false;
            sack.bitmask = (bt::Uint8 *)ptr + 2;
        }

        data_off += 2 + ptr[1];
        ext_id = ptr[0];
    }

    data_size = size - data_off;
    return true;
}

const utp::SelectiveAck *PacketParser::selectiveAck() const
{
    return sack_found ? &sack : nullptr;
}

}
