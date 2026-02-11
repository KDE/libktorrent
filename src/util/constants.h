/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCONSTANTS_H
#define BTCONSTANTS_H

#include <QtGlobal>

namespace bt
{
using Uint64 = quint64;
using Uint32 = quint32;
using Uint16 = quint16;
using Uint8 = quint8;

using Int64 = qint64;
using Int32 = qint32;
using Int16 = qint16;
using Int8 = qint8;

using TimeStamp = Uint64;

enum Priority {
    // also leave some room if we want to add new priorities in the future
    FIRST_PREVIEW_PRIORITY = 55,
    FIRST_PRIORITY = 50,
    NORMAL_PREVIEW_PRIORITY = 45,
    NORMAL_PRIORITY = 40,
    LAST_PREVIEW_PRIORITY = 35,
    LAST_PRIORITY = 30,
    ONLY_SEED_PRIORITY = 20,
    EXCLUDED = 10,
};

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
