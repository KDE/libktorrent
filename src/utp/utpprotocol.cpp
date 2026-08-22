/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "utpprotocol.h"

#include <util/endian.h>

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

void Header::read(QByteArrayView data)
{
    const auto first_byte = bt::ReadUint8(data, 0);
    type = (first_byte & 0xF0) >> 4;
    version = first_byte & 0x0F;
    extension = bt::ReadUint8(data, 1);
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

PacketParser::PacketParser(QByteArrayView pkt)
    : packet(pkt)
    , sack_found(false)
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
    if (packet.size() < Header::size()) {
        return false;
    }

    data_off = Header::size();
    auto remaining_packet = packet.sliced(Header::size());

    // go over all header extensions to increase the data offset and watch out for selective acks
    int ext_id = hdr.extension;
    while (!remaining_packet.isEmpty() && ext_id != 0) {
        if (remaining_packet.size() < 2) {
            return false;
        }
        const auto next_ext_id = bt::ReadUint8(remaining_packet, 0);
        const auto ext_length = bt::ReadUint8(remaining_packet, 1);
        remaining_packet.slice(2);

        if (ext_id == SELECTIVE_ACK_ID) {
            sack_found = true;
            sack.extension = next_ext_id;
            sack.length = ext_length;
            if (sack.length > packet.size()) {
                return false;
            }
            // NOTE: sack.bitmask is of non-const type as the same struct is used for sending data.
            // We only return a const pointer to sack so there is no non-const access to the bitmask and thus no undefined behaviour.
            sack.bitmask = const_cast<bt::Uint8 *>(reinterpret_cast<const bt::Uint8 *>(remaining_packet.data()));
        }

        ext_id = next_ext_id;
    }

    data_off = packet.size() - remaining_packet.size();
    data_size = remaining_packet.size();
    return true;
}

const utp::SelectiveAck *PacketParser::selectiveAck() const
{
    return sack_found ? &sack : nullptr;
}

}
