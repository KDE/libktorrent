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

    if (addr.ipVersion() == 4) {
        // first check size
        if ((int)off + 26 > ba.size())
            throw bt::Error(u"Not enough room in buffer"_s);

        // copy ID, IP address and port into the buffer
        memcpy(ptr, e.getID().getData(), 20);
        bt::WriteUint32(ptr, 20, addr.toIPv4Address());
        bt::WriteUint16(ptr, 24, addr.port());
    } else {
        // first check size
        if ((int)off + 38 > ba.size())
            throw bt::Error(u"Not enough room in buffer"_s);

        // copy ID, IP address and port into the buffer
        memcpy(ptr, e.getID().getData(), 20);
        memcpy(ptr + 20, addr.toIPv6Address().c, 16);
        bt::WriteUint16(ptr, 36, addr.port());
    }
}

KBucketEntry UnpackBucketEntry(const QByteArray &ba, Uint32 off, int ip_version)
{
    if (ip_version == 4) {
        if ((int)off + 26 > ba.size())
            throw bt::Error(u"Not enough room in buffer"_s);

        const Uint8 *data = (Uint8 *)ba.data();
        const Uint8 *ptr = data + off;

        // get the port, ip and key);
        Uint16 port = bt::ReadUint16(ptr, 24);
        Uint32 ip = bt::ReadUint32(ptr, 20);
        Uint8 key[20];
        memcpy(key, ptr, 20);

        return KBucketEntry(net::Address(ip, port), dht::Key(key));
    } else {
        if ((int)off + 38 > ba.size())
            throw bt::Error(u"Not enough room in buffer"_s);

        const Uint8 *data = (Uint8 *)ba.data();
        const Uint8 *ptr = data + off;

        // get the port, ip and key);
        Uint16 port = bt::ReadUint16(ptr, 36);
        Uint8 key[20];
        memcpy(key, ptr, 20);

        return KBucketEntry(net::Address((quint8 *)ptr + 20, port), dht::Key(key));
    }
}

}
