/*
    SPDX-FileCopyrightText: 2025 Andrius Štikonas <andrius@stikonas.eu>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "infohash.h"
#include "urlencoder.h"

#include <QString>

namespace bt
{
InfoHash::InfoHash()
{
}

InfoHash::InfoHash(SHA1Hash v1, SHA2Hash v2)
    : v1(v1)
    , v2(v2)
{
}

InfoHash::InfoHash(const InfoHash &other)
{
    v1 = other.v1;
    v2 = other.v2;
}

InfoHash &InfoHash::operator=(const InfoHash &other)
{
    v1 = other.v1;
    v2 = other.v2;
    return *this;
}

bool InfoHash::operator!=(const InfoHash &other) const
{
    return (v1 != other.v1) || (v2 != other.v2);
}

bool InfoHash::operator==(const InfoHash &other) const
{
    return (v1 == other.v1) && (v2 == other.v2);
}

bool InfoHash::hasV1() const
{
    return !(v1 == SHA1Hash());
}

bool InfoHash::hasV2() const
{
    return !(v2 == SHA2Hash());
}

SHA1Hash InfoHash::getV1() const
{
    return v1;
}

SHA2Hash InfoHash::getV2() const
{
    return v2;
}

SHA1Hash InfoHash::truncated() const
{
    return hasV2() ? SHA1Hash(v2.getData()) : v1;
}

QString InfoHash::toString() const
{
    return hasV2() ? v2.toString() : v1.toString();
}

QString InfoHash::toURLString() const
{
    return URLEncoder::encode((const char *)truncated().getData(), 32);
}

bool InfoHash::containsTruncatedV2(const SHA1Hash &truncated_hash) const
{
    return memcmp(v2.getData(), truncated_hash.getData(), 20) == 0;
}

size_t qHash(const InfoHash &key, size_t seed = 0) noexcept
{
    return qHash(key.truncated(), seed);
}

}
