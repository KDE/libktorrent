/*
    SPDX-FileCopyrightText: 2024 Andrius Štikonas <andrius@stikonas.eu>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "sha2hash.h"
#include "log.h"
#include "sha2hashgen.h"
#include "urlencoder.h"
#include <QHash>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace bt
{
SHA2Hash::SHA2Hash()
{
    memset(hash, 0, 32);
}

SHA2Hash::SHA2Hash(const SHA2Hash &other)
{
    memcpy(hash, other.hash, 32);
}

SHA2Hash::SHA2Hash(QByteArrayView h)
{
    memcpy(hash, h.data(), 32);
}

SHA2Hash::SHA2Hash(const Uint8 *h)
{
    memcpy(hash, h, 32);
}

SHA2Hash::~SHA2Hash()
{
}

SHA2Hash &SHA2Hash::operator=(const SHA2Hash &other)
{
    memcpy(hash, other.hash, 32);
    return *this;
}

bool SHA2Hash::operator==(const SHA2Hash &other) const
{
    return memcmp(hash, other.hash, 32) == 0;
}

SHA2Hash SHA2Hash::generate(const QByteArrayView data)
{
    SHA2HashGen hg;

    return hg.generate(data);
}

SHA2Hash SHA2Hash::generate(const Uint8 *data, Uint32 len)
{
    return generate(QByteArrayView(data, len));
}

#define hex_str '%', '0', '2', 'x'
#define hex_str4 hex_str, hex_str, hex_str, hex_str
#define hex_str32 hex_str4, hex_str4, hex_str4, hex_str4, hex_str4, hex_str4, hex_str4, hex_str4
QString SHA2Hash::toString() const
{
    char tmp[65];
    char fmt[129] = {hex_str32, '\0'};
    const Uint8 *h = getData();
    snprintf(tmp,
             65,
             fmt,
             h[0],
             h[1],
             h[2],
             h[3],
             h[4],
             h[5],
             h[6],
             h[7],
             h[8],
             h[9],
             h[10],
             h[11],
             h[12],
             h[13],
             h[14],
             h[15],
             h[16],
             h[17],
             h[18],
             h[19],
             h[20],
             h[21],
             h[22],
             h[23],
             h[24],
             h[25],
             h[26],
             h[27],
             h[28],
             h[29],
             h[30],
             h[31]);
    return QString::fromLatin1(tmp, 64);
}

QByteArray SHA2Hash::toByteArray() const
{
    return QByteArray((const char *)hash, 32);
}

QString SHA2Hash::toURLString() const
{
    return URLEncoder::encode((const char *)hash, 20);
}

Log &operator<<(Log &out, const SHA2Hash &h)
{
    out << h.toString();
    return out;
}

SHA2Hash operator^(const SHA2Hash &a, const SHA2Hash &b)
{
    SHA2Hash k;
    Uint64 *k64 = (Uint64 *)k.hash;
    const Uint64 *a64 = (Uint64 *)a.hash;
    const Uint64 *b64 = (Uint64 *)b.hash;
    for (unsigned i = 0; i < 4; ++i) {
        k64[i] = a64[i] ^ b64[i];
    }
    return k;
}

bool operator<(const SHA2Hash &a, const SHA2Hash &b)
{
    return memcmp(a.hash, b.hash, 32) < 0;
}

size_t qHash(const SHA2Hash &key, size_t seed = 0) noexcept
{
    return qHash(key.toByteArray(), seed);
}
}
