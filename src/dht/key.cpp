/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include "key.h"

#include <QRandomGenerator>

#include <algorithm>
#include <util/constants.h>

#if defined(Q_OS_FREEBSD) || defined(Q_OS_NETBSD)
#include <sys/endian.h>
#else
#include <endian.h>
#endif

using namespace bt;

namespace dht
{
Key::Key()
{
}

Key::Key(const bt::SHA1Hash &k)
    : bt::SHA1Hash(k)
{
}

Key::Key(const bt::Uint8 *d)
    : bt::SHA1Hash(d)
{
}

Key::Key(const QByteArray &ba)
{
    memcpy(hash, ba.data(), std::min(20, ba.size()));
}

Key::~Key()
{
}

bool Key::operator==(const Key &other) const
{
    return bt::SHA1Hash::operator==(other);
}

bool Key::operator!=(const Key &other) const
{
    return !operator==(other);
}

bool Key::operator<(const Key &other) const
{
    return memcmp(hash, other.hash, 20) < 0;
}

bool Key::operator<=(const Key &other) const
{
    return memcmp(hash, other.hash, 20) <= 0;
}

bool Key::operator>(const Key &other) const
{
    return memcmp(hash, other.hash, 20) > 0;
}

bool Key::operator>=(const Key &other) const
{
    return memcmp(hash, other.hash, 20) >= 0;
}

Key operator+(const dht::Key &a, const dht::Key &b)
{
    dht::Key result;
    bt::Uint64 sum = 0;

    for (int i = 4; i >= 0; i--) {
        sum += (bt::Uint64)htobe32(a.hash[i]) + htobe32(b.hash[i]);
        result.hash[i] = htobe32(sum & 0xFFFFFFFF);
        sum = sum >> 32;
    }

    return result;
}

Key operator+(const Key &a, bt::Uint8 value)
{
    dht::Key result(a);

    bt::Uint64 sum = value;
    for (int i = 4; i >= 0 && sum != 0; i--) {
        sum += htobe32(result.hash[i]);
        result.hash[i] = htobe32(sum & 0xFFFFFFFF);
        sum = sum >> 32;
    }

    return result;
}

Key Key::operator/(int value) const
{
    dht::Key result;
    bt::Uint64 remainder = 0;

    for (int i = 0; i < 5; i++) {
        const bt::Uint32 h = htobe32(hash[i]);
        result.hash[i] = htobe32((h + remainder) / value);
        remainder = ((h + remainder) % value) << 32;
    }

    return result;
}

Key Key::distance(const Key &a, const Key &b)
{
    return a ^ b;
}

Key Key::random()
{
    Key k;
    Uint32 *h = k.hash;
    for (int i = 0; i < 5; i++) {
        h[i] = QRandomGenerator::global()->generate();
    }
    return k;
}

Key operator-(const Key &a, const Key &b)
{
    dht::Key result;
    bt::Uint32 carry = 0;
    for (int i = 4; i >= 0; i--) {
        const bt::Uint32 a32 = htobe32(a.hash[i]);
        const bt::Uint32 b32 = htobe32(b.hash[i]);
        if (a32 >= ((bt::Uint64)b32 + carry)) {
            result.hash[i] = htobe32(a32 - b32 - carry);
            carry = 0;
        } else {
            const bt::Uint64 max = 0xFFFFFFFF + 1;
            result.hash[i] = htobe32((bt::Uint32)(max - (b32 - a32) - carry));
            carry = 1;
        }
    }

    return result;
}

Key Key::mid(const dht::Key &a, const dht::Key &b)
{
    if (a <= b)
        return a + (b - a) / 2;
    else
        return b + (a - b) / 2;
}

#define rep4(v) v, v, v, v
#define rep20(v) rep4(v), rep4(v), rep4(v), rep4(v), rep4(v)
const bt::Uint8 val_max[20] = {rep20(0xFF)};
const bt::Uint8 val_min[20] = {rep20(0x00)};

Key Key::max()
{
    return Key(val_max);
}

Key Key::min()
{
    return Key(val_min);
}

}
