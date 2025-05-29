/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_UTPPROTOCOL_H
#define UTP_UTPPROTOCOL_H

#include <QString>
#include <QtGlobal>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace utp
{
/*
UTP header:

0       4       8               16              24              32
+-------+-------+---------------+---------------+---------------+
| ver   | type  | extension     | connection_id                 |
+-------+-------+---------------+---------------+---------------+
| timestamp_microseconds                                        |
+---------------+---------------+---------------+---------------+
| timestamp_difference_microseconds                             |
+---------------+---------------+---------------+---------------+
| wnd_size                                                      |
+---------------+---------------+---------------+---------------+
| seq_nr                        | ack_nr                        |
+---------------+---------------+---------------+---------------+
*/

struct KTORRENT_EXPORT Header {
    unsigned int version : 4;
    unsigned int type : 4;
    bt::Uint8 extension;
    bt::Uint16 connection_id;
    bt::Uint32 timestamp_microseconds;
    bt::Uint32 timestamp_difference_microseconds;
    bt::Uint32 wnd_size;
    bt::Uint16 seq_nr;
    bt::Uint16 ack_nr;

    void read(const bt::Uint8 *data);
    void write(bt::Uint8 *data) const;
    static bt::Uint32 size();
};

struct SelectiveAck {
    bt::Uint8 *bitmask;
    bt::Uint8 extension;
    bt::Uint8 length;

    SelectiveAck()
        : bitmask(nullptr)
        , extension(0)
        , length(0)
    {
    }
};

struct ExtensionBits {
    bt::Uint8 extension;
    bt::Uint8 length;
    bt::Uint8 extension_bitmask[8];
};

struct UnknownExtension {
    bt::Uint8 extension;
    bt::Uint8 length;
};

const bt::Uint8 SELECTIVE_ACK_ID = 1;
const bt::Uint8 EXTENSION_BITS_ID = 2;

// type field values
const bt::Uint8 ST_DATA = 0;
const bt::Uint8 ST_FIN = 1;
const bt::Uint8 ST_STATE = 2;
const bt::Uint8 ST_RESET = 3;
const bt::Uint8 ST_SYN = 4;

KTORRENT_EXPORT QString TypeToString(bt::Uint8 type);

enum ConnectionState {
    CS_IDLE,
    CS_SYN_SENT,
    CS_CONNECTED,
    CS_FINISHED,
    CS_CLOSED,
};

const bt::Uint32 MIN_PACKET_SIZE = 150;
const bt::Uint32 MAX_PACKET_SIZE = 16384;

const bt::Uint32 DELAY_WINDOW_SIZE = 2 * 60 * 1000; // 2 minutes
const bt::Uint32 CCONTROL_TARGET = 100;
const bt::Uint32 MAX_CWND_INCREASE_PACKETS_PER_RTT = 500;
const bt::Uint32 MAX_TIMEOUT = 30000;
const bt::Uint32 CONNECT_TIMEOUT = 30000;
const bt::Uint32 KEEP_ALIVE_TIMEOUT = 30000;

const bt::Uint32 IP_AND_UDP_OVERHEAD = 28;

/*
 Test if a bit is acked
UTP standard:
The bitmask has reverse byte order. The first byte represents packets [ack_nr + 2, ack_nr + 2 + 7] in reverse order. The least significant bit in the byte
represents ack_nr + 2, the most significant bit in the byte represents ack_nr + 2 + 7. The next byte in the mask represents [ack_nr + 2 + 8, ack_nr + 2 + 15] in
reverse order, and so on. The bitmask is not limited to 32 bits but can be of any size.

Here is the layout of a bitmask representing the first 32 packet acks represented in a selective ACK bitfield:

0               8               16
+---------------+---------------+---------------+---------------+
| 9 8 ...   3 2 | 17   ...   10 | 25   ...   18 | 33   ...   26 |
+---------------+---------------+---------------+---------------+

The number in the diagram maps the bit in the bitmask to the offset to add to ack_nr in order to calculate the sequence number that the bit is ACKing.
*/
inline bool Acked(const SelectiveAck *sack, bt::Uint16 bit)
{
    // check bounds
    if (bit < 2 || bit > 8 * sack->length + 1)
        return false;

    const bt::Uint8 *bitset = sack->bitmask;
    int byte = (bit - 2) / 8;
    int bit_off = (bit - 2) % 8;
    return bitset[byte] & (0x01 << bit_off);
}

// Turn on a bit in the SelectiveAck
inline void Ack(SelectiveAck *sack, bt::Uint16 bit)
{
    // check bounds
    if (bit < 2 || bit > 8 * sack->length + 1)
        return;

    bt::Uint8 *bitset = sack->bitmask;
    int byte = (bit - 2) / 8;
    int bit_off = (bit - 2) % 8;
    bitset[byte] |= (0x01 << bit_off);
}

/*!
    Helper class to parse packets
*/
class KTORRENT_EXPORT PacketParser
{
public:
    PacketParser(const QByteArray &packet);
    PacketParser(const bt::Uint8 *packet, bt::Uint32 size);
    ~PacketParser();

    //! Parses the packet, returns false on error
    bool parse();

    const Header *header() const
    {
        return &hdr;
    }
    const SelectiveAck *selectiveAck() const;
    bt::Uint32 dataOffset() const
    {
        return data_off;
    }
    bt::Uint32 dataSize() const
    {
        return data_size;
    }

private:
    const bt::Uint8 *packet;
    Header hdr;
    SelectiveAck sack;
    bool sack_found;
    bt::Uint32 size;
    bt::Uint32 data_off;
    bt::Uint32 data_size;
};

//! <= for sequence numbers (taking into account wrapping around)
inline bool SeqNrCmpSE(bt::Uint16 a, bt::Uint16 b)
{
    if (qAbs(b - a) < 32768)
        return a <= b;
    else
        return a > b;
}

//! < for sequence numbers (taking into account wrapping around)
inline bool SeqNrCmpS(bt::Uint16 a, bt::Uint16 b)
{
    if (qAbs(b - a) < 32768)
        return a < b;
    else
        return a > b;
}

//! Calculates difference between two sequence numbers (taking into account wrapping around)
inline bt::Uint16 SeqNrDiff(bt::Uint16 a, bt::Uint16 b)
{
    if (qAbs(b - a) < 32768)
        return b - a;
    else if (b > a)
        return a + (65536 - b);
    else
        return b + (65536 - a);
}
}

#endif // UTP_UTPPROTOCOL_H
