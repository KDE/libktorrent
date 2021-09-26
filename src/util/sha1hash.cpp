/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "sha1hash.h"
#include "log.h"
#include "sha1hashgen.h"
#include "urlencoder.h"
#include <QHash>
#include <algorithm>
#include <stdio.h>
#include <string.h>

namespace bt
{
SHA1Hash::SHA1Hash()
{
    memset(hash, 0, 20);
}

SHA1Hash::SHA1Hash(const SHA1Hash &other)
{
    memcpy(hash, other.hash, 20);
}

SHA1Hash::SHA1Hash(const Uint8 *h)
{
    memcpy(hash, h, 20);
}

SHA1Hash::~SHA1Hash()
{
}

SHA1Hash &SHA1Hash::operator=(const SHA1Hash &other)
{
    memcpy(hash, other.hash, 20);
    return *this;
}

bool SHA1Hash::operator==(const SHA1Hash &other) const
{
    return memcmp(hash, other.hash, 20) == 0;
}

SHA1Hash SHA1Hash::generate(const Uint8 *data, Uint32 len)
{
    SHA1HashGen hg;

    return hg.generate(data, len);
}

#define hex_str '%', '0', '2', 'x'
#define hex_str4 hex_str, hex_str, hex_str, hex_str
#define hex_str20 hex_str4, hex_str4, hex_str4, hex_str4, hex_str4
QString SHA1Hash::toString() const
{
    char tmp[41];
    char fmt[81] = {hex_str20, '\0'};
    const Uint8 *h = getData();
    snprintf(tmp, 41, fmt, h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7], h[8], h[9], h[10], h[11], h[12], h[13], h[14], h[15], h[16], h[17], h[18], h[19]);
    return QString::fromLatin1(tmp, 40);
}

QByteArray SHA1Hash::toByteArray() const
{
    return QByteArray((const char *)hash, 20);
}

QString SHA1Hash::toURLString() const
{
    return URLEncoder::encode((const char *)hash, 20);
}

Log &operator<<(Log &out, const SHA1Hash &h)
{
    out << h.toString();
    return out;
}

SHA1Hash operator^(const SHA1Hash &a, const SHA1Hash &b)
{
    SHA1Hash k;
    Uint64 *k64 = (Uint64 *)k.hash;
    const Uint64 *a64 = (Uint64 *)a.hash;
    const Uint64 *b64 = (Uint64 *)b.hash;
    k64[0] = a64[0] ^ b64[0];
    k64[1] = a64[1] ^ b64[1];
    k.hash[4] = a.hash[4] ^ b.hash[4];
    return k;
}

bool operator<(const SHA1Hash &a, const SHA1Hash &b)
{
    return memcmp(a.hash, b.hash, 20) < 0;
}

uint qHash(const SHA1Hash &key)
{
    return qHash(key.toByteArray());
}
}
