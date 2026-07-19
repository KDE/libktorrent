/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "bitset.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace bt
{
namespace
{
constexpr auto NumBytesFromNumBits(const auto num_bits)
{
    return (num_bits >> 3) + (((num_bits & 7) > 0) ? 1 : 0);
}
}

BitSet BitSet::null;

BitSet::BitSet(Uint32 num_bits)
    : num_bits(num_bits)
    , data(NumBytesFromNumBits(num_bits))
{
}

BitSet::BitSet(const Uint8 *d, Uint32 num_bits)
    : num_bits(num_bits)
{
    data.assign(d, d + NumBytesFromNumBits(num_bits));
    updateNumOnBits();
}

BitSet::BitSet(BitSet &&bs)
    : num_bits(std::exchange(bs.num_bits, 0))
    , data(std::move(bs.data))
    , num_on(std::exchange(bs.num_on, 0))
{
}

void BitSet::updateNumOnBits()
{
    num_on = 0;
    Uint32 i = 0;
    while (i < data.size()) {
        num_on += std::popcount(data[i]);
        i++;
    }
}

BitSet &BitSet::operator=(BitSet &&bs)
{
    num_bits = std::exchange(bs.num_bits, 0);
    data = std::move(bs.data);
    num_on = std::exchange(bs.num_on, 0);
    return *this;
}

const Uint8 tail_mask_lookup[8] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};

void BitSet::invert()
{
    if (data.size() <= 0) {
        return;
    }

    num_on = 0;
    Uint32 i = 0;
    while (i < data.size() - 1) {
        data[i] = ~data[i];
        num_on += std::popcount(data[i]);
        i++;
    }
    // i == data.size()-1
    data[i] = ~data[i] & tail_mask_lookup[num_bits & 7];
    num_on += std::popcount(data[i]);
}

BitSet &BitSet::operator-=(const BitSet &bs)
{
    num_on = 0;
    for (Uint32 i = 0; i < data.size(); i++) {
        data[i] &= ~(data[i] & bs.data[i]);
        num_on += std::popcount(data[i]);
    }
    return *this;
}

BitSet BitSet::operator-(const BitSet &bs) const
{
    return BitSet(*this) -= bs;
}

void BitSet::setAll(bool on)
{
    data.fill(on ? 0xFF : 0x00);
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
        for (Uint32 i = 0; i < data.size(); i++) {
            data[i] |= other.data[i];
            num_on += std::popcount(data[i]);
        }
        return;
    }

    // process till the end of other data or last-1 byte in our data
    // whatether comes first
    for (Uint32 i = 0; i < qMin(data.size() - 1, other.data.size()); i++) {
        data[i] |= other.data[i];
        num_on += std::popcount(data[i]);
    }

    // if last-1 not reached yet then the end of other data is reached
    // so just add std::popcount till last-1 byte
    for (Uint32 i = other.data.size(); i < data.size() - 1; i++) {
        num_on += std::popcount(data[i]);
    }

    // if other has matching byte for our last byte - OR it with proper mask
    if (other.data.size() >= data.size()) {
        data[data.size() - 1] = (data[data.size() - 1] | other.data[data.size() - 1]) & tail_mask_lookup[data.size() & 7];
    }

    // count bits set in last byte
    num_on += std::popcount(data[data.size() - 1]);
}

void BitSet::andBitSet(const BitSet &other)
{
    num_on = 0;

    if (num_bits == other.num_bits) {
        // best case
        for (Uint32 i = 0; i < data.size(); i++) {
            data[i] &= other.data[i];
            num_on += std::popcount(data[i]);
        }
        return;
    }

    // we expect 0's at the tail of last byte (if any)
    // so just AND matching bytes and clear the others
    // no need to worry about mask for last byte
    for (Uint32 i = 0; i < qMin(data.size(), other.data.size()); i++) {
        data[i] &= other.data[i];
        num_on += std::popcount(data[i]);
    }

    if (data.size() > other.data.size()) {
        std::fill(data.begin() + other.data.size(), data.end(), 0);
    }
}

bool BitSet::includesBitSet(const BitSet &other) const
{
    if (num_bits == other.num_bits) {
        // best case
        for (Uint32 i = 0; i < data.size(); i++) {
            if ((data[i] | other.data[i]) != data[i]) {
                return false;
            }
        }
        return true;
    }

    // process till the end of other data or last-1 byte in our data
    // whatether comes first
    for (Uint32 i = 0; i < qMin(data.size() - 1, other.data.size()); i++) {
        if ((data[i] | other.data[i]) != data[i]) {
            return false;
        }
    }

    // if other has matching byte for our last byte - OR it with proper mask
    if (other.data.size() >= data.size()) {
        const Uint8 d = data[data.size() - 1];
        if (((d | other.data[data.size() - 1]) & tail_mask_lookup[data.size() & 7]) != d) {
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

    return data == bs.data;
}
}
