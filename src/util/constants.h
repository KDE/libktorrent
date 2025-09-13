/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCONSTANTS_H
#define BTCONSTANTS_H

#include <QtGlobal>

namespace bt
{
typedef quint64 Uint64;
typedef quint32 Uint32;
typedef quint16 Uint16;
typedef quint8 Uint8;

typedef qint64 Int64;
typedef qint32 Int32;
typedef qint16 Int16;
typedef qint8 Int8;

typedef Uint64 TimeStamp;

typedef enum {
    // also leave some room if we want to add new priorities in the future
    FIRST_PREVIEW_PRIORITY = 55,
    FIRST_PRIORITY = 50,
    NORMAL_PREVIEW_PRIORITY = 45,
    NORMAL_PRIORITY = 40,
    LAST_PREVIEW_PRIORITY = 35,
    LAST_PRIORITY = 30,
    ONLY_SEED_PRIORITY = 20,
    EXCLUDED = 10,
} Priority;

const Uint32 MAX_MSGLEN = 9 + 131072;
const Uint16 MIN_PORT = 6881;
const Uint16 MAX_PORT = 6889;
const Uint32 MAX_PIECE_LEN = 16384;

enum PeerMessageType : Uint8 {
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8,
    // DHT Protocol, bep 0005
    PORT = 9,
    // Fast Extension, bep 0006
    SUGGEST_PIECE = 13,
    HAVE_ALL = 14,
    HAVE_NONE = 15,
    REJECT_REQUEST = 16,
    ALLOWED_FAST = 17,
    // Extension Protocol, bep 0010
    EXTENDED = 20,
};

// flags for things which a peer supports
const Uint32 DHT_SUPPORT = 0x01;
const Uint32 EXT_PROT_SUPPORT = 0x10;
const Uint32 FAST_EXT_SUPPORT = 0x04;

enum TransportProtocol { TCP, UTP };
}

#endif
