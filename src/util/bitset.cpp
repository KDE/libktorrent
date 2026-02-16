/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "bitset.h"
#include <algorithm>
#include <cstring>

namespace bt
{
BitSet BitSet::null;

BitSet::BitSet(Uint32 num_bits)
    : num_bits(num_bits)
    , num_bytes((num_bits >> 3) + (((num_bits & 7) > 0) ? 1 : 0))
    , data(new Uint8[num_bytes])
    , num_on(0)
{
    std::fill(data, data + num_bytes, 0x00);
}

BitSet::BitSet(const Uint8 *d, Uint32 num_bits)
    : num_bits(num_bits)
    , num_bytes((num_bits >> 3) + (((num_bits & 7) > 0) ? 1 : 0))
    , data(new Uint8[num_bytes])
    , num_on(0)
{
    memcpy(data, d, num_bytes);
    updateNumOnBits();
}

BitSet::BitSet(const BitSet &bs)
    : num_bits(bs.num_bits)
    , num_bytes(bs.num_bytes)
    , data(new Uint8[num_bytes])
    , num_on(bs.num_on)
{
    std::copy(bs.data, bs.data + num_bytes, data);
}

BitSet::~BitSet()
{
    delete[] data;
}

void BitSet::updateNumOnBits()
{
    num_on = 0;
    Uint32 i = 0;
    while (i < num_bytes) {
        num_on += BitCount[data[i]];
        i++;
    }
}

BitSet &BitSet::operator=(const BitSet &bs)
{
    delete[] data;
    num_bytes = bs.num_bytes;
    num_bits = bs.num_bits;
    data = new Uint8[num_bytes];
    std::copy(bs.data, bs.data + num_bytes, data);
    num_on = bs.num_on;
    return *this;
}

const Uint8 tail_mask_lookup[8] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};

void BitSet::invert()
{
    if (num_bytes <= 0) {
        return;
    }

    num_on = 0;
    Uint32 i = 0;
    while (i < num_bytes - 1) {
        data[i] = ~data[i];
        num_on += BitCount[data[i]];
        i++;
    }
    // i == num_bytes-1
    data[i] = ~data[i] & tail_mask_lookup[num_bits & 7];
    num_on += BitCount[data[i]];
}

BitSet &BitSet::operator-=(const BitSet &bs)
{
    num_on = 0;
    for (Uint32 i = 0; i < num_bytes; i++) {
        data[i] &= ~(data[i] & bs.data[i]);
        num_on += BitCount[data[i]];
    }
    return *this;
}

BitSet BitSet::operator-(const BitSet &bs) const
{
    return BitSet(*this) -= bs;
}

void BitSet::setAll(bool on)
{
    std::fill(data, data + num_bytes, on ? 0xFF : 0x00);
    num_on = on ? num_bits : 0;
}

void BitSet::clear()
{
    setAll(false);
}

void BitSet::orBitSet(const BitSet &other)
{
    num_on = 0;

    if (num_bits == other.num_bits) {
        // best case
        for (Uint32 i = 0; i < num_bytes; i++) {
            data[i] |= other.data[i];
            num_on += BitCount[data[i]];
        }
        return;
    }

    // process till the end of other data or last-1 byte in our data
    // whatether comes first
    for (Uint32 i = 0; i < qMin(num_bytes - 1, other.num_bytes); i++) {
        data[i] |= other.data[i];
        num_on += BitCount[data[i]];
    }

    // if last-1 not reached yet then the end of other data is reached
    // so just add BitCount till last-1 byte
    for (Uint32 i = other.num_bytes; i < num_bytes - 1; i++) {
        num_on += BitCount[data[i]];
    }

    // if other has matching byte for our last byte - OR it with proper mask
    if (other.num_bytes >= num_bytes) {
        data[num_bytes - 1] = (data[num_bytes - 1] | other.data[num_bytes - 1]) & tail_mask_lookup[num_bytes & 7];
    }

    // count bits set in last byte
    num_on += BitCount[data[num_bytes - 1]];
}

void BitSet::andBitSet(const BitSet &other)
{
    num_on = 0;

    if (num_bits == other.num_bits) {
        // best case
        for (Uint32 i = 0; i < num_bytes; i++) {
            data[i] &= other.data[i];
            num_on += BitCount[data[i]];
        }
        return;
    }

    // we expect 0's at the tail of last byte (if any)
    // so just AND matching bytes and clear the others
    // no need to worry about mask for last byte
    for (Uint32 i = 0; i < qMin(num_bytes, other.num_bytes); i++) {
        data[i] &= other.data[i];
        num_on += BitCount[data[i]];
    }

    if (num_bytes > other.num_bytes) {
        memset(data + other.num_bytes, 0, num_bytes - other.num_bytes);
    }
}

bool BitSet::includesBitSet(const BitSet &other) const
{
    if (num_bits == other.num_bits) {
        // best case
        for (Uint32 i = 0; i < num_bytes; i++) {
            if ((data[i] | other.data[i]) != data[i]) {
                return false;
            }
        }
        return true;
    }

    // process till the end of other data or last-1 byte in our data
    // whatether comes first
    for (Uint32 i = 0; i < qMin(num_bytes - 1, other.num_bytes); i++) {
        if ((data[i] | other.data[i]) != data[i]) {
            return false;
        }
    }

    // if other has matching byte for our last byte - OR it with proper mask
    if (other.num_bytes >= num_bytes) {
        const Uint8 d = data[num_bytes - 1];
        if (((d | other.data[num_bytes - 1]) & tail_mask_lookup[num_bytes & 7]) != d) {
            return false;
        }
    }

    return true;
}

bool BitSet::allOn() const
{
    return num_on == num_bits;
}

bool BitSet::operator==(const BitSet &bs) const
{
    if (this->getNumBits() != bs.getNumBits()) {
        return false;
    }

    return memcmp(data, bs.data, num_bytes) == 0;
}
}
