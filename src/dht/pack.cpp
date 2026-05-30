/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "pack.h"
#include <util/error.h>
#include <util/functions.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
void PackBucketEntry(const KBucketEntry &e, QByteArray &ba, Uint32 off)
{
    const net::Address &addr = e.getAddress();
    Uint8 *data = (Uint8 *)ba.data();
    Uint8 *ptr = data + off;

    // first check size
    if (addr.ipVersion() == 4) {
        if ((int)off + 26 > ba.size()) {
            throw bt::Error(u"Not enough room in buffer"_s);
        }
    } else {
        if ((int)off + 38 > ba.size()) {
            throw bt::Error(u"Not enough room in buffer"_s);
        }
    }

    // copy ID, IP address and port into the buffer
    memcpy(ptr, e.getID().getData(), 20);
    addr.writeCompact(ptr + 20);
}

KBucketEntry UnpackBucketEntry(QByteArrayView ba, int ip_version)
{
    if (ip_version == 4) {
        if (ba.size() < 26) {
            throw bt::Error(u"Not enough room in buffer"_s);
        }

        return KBucketEntry(net::Address::fromCompactIPv4(ba.sliced(20)), dht::Key(ba.first(20)));
    } else {
        if (ba.size() < 38) {
            throw bt::Error(u"Not enough room in buffer"_s);
        }

        return KBucketEntry(net::Address::fromCompactIPv6(ba.sliced(20)), dht::Key(ba.first(20)));
    }
}

}
