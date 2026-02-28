/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTBITSET_H
#define BTBITSET_H

#include <bit>

#include "constants.h"
#include <ktorrent_export.h>

namespace bt
{
/*!
 * \headerfile util/bitset.h
 * \author Joris Guisson
 * \brief Represents a fixed-size sequence of bits.
 *
 * Simple implementation of a BitSet, can only turn on and off bits.
 * BitSet's are used to indicate which chunks we have or not.
 */
class KTORRENT_EXPORT BitSet
{
    Uint32 num_bits, num_bytes;
    Uint8 *data;
    Uint32 num_on;

public:
    /*!
     * Constructor.
     * \param num_bits The number of bits
     */
    BitSet(Uint32 num_bits = 8);

    /*!
     * Manually set data.
     * \param data The data
     * \param num_bits The number of bits
     */
    BitSet(const Uint8 *data, Uint32 num_bits);

    /*!
     * Copy constructor.
     * \param bs BitSet to copy
     */
    BitSet(const BitSet &bs);
    virtual ~BitSet();

    //! See if the BitSet is null
    [[nodiscard]] bool isNull() const
    {
        return num_bits == 0;
    }

    /*!
     * Get the value of a bit, false means 0, true 1.
     * \param i Index of Bit
     */
    [[nodiscard]] bool get(Uint32 i) const;

    /*!
     * Set the value of a bit, false means 0, true 1.
     * \param i Index of Bit
     * \param on False means 0, true 1
     */
    void set(Uint32 i, bool on);

    //! Set all bits on or off
    void setAll(bool on);

    [[nodiscard]] Uint32 getNumBytes() const
    {
        return num_bytes;
    }
    [[nodiscard]] Uint32 getNumBits() const
    {
        return num_bits;
    }
    [[nodiscard]] const Uint8 *getData() const
    {
        return data;
    }
    Uint8 *getData()
    {
        return data;
    }

    //! Get the number of on bits
    [[nodiscard]] Uint32 numOnBits() const
    {
        return num_on;
    }

    /*!
     * Set all bits to 0
     */
    void clear();

    /*!
     * invert this BitSet
     */
    void invert();

    /*!
     * or this BitSet with another.
     * \param other The other BitSet
     */
    void orBitSet(const BitSet &other);

    /*!
     * and this BitSet with another.
     * \param other The other BitSet
     */
    void andBitSet(const BitSet &other);

    /*!
     * see if this BitSet includes another.
     * \param other The other BitSet
     */
    [[nodiscard]] bool includesBitSet(const BitSet &other) const;

    /*!
     * Assignment operator.
     * \param bs BitSet to copy
     * \return *this
     */
    BitSet &operator=(const BitSet &bs);

    /*!
     * Subtraction assignment operator.
     * \param bs BitSet to copy and subtract from this one
     * \return *this
     */
    BitSet &operator-=(const BitSet &bs);

    /*!
     * Subtraction operator.
     * \param bs BitSet to subtract from this one
     * \return difference
     */
    BitSet operator-(const BitSet &bs) const;

    //! Check if all bit are set to 1
    [[nodiscard]] bool allOn() const;

    /*!
     * Check for equality of bitsets
     * \param bs BitSet to compare
     * \return true if equal
     */
    bool operator==(const BitSet &bs) const;

    /*!
     * Opposite of operator ==
     */
    bool operator!=(const BitSet &bs) const
    {
        return !operator==(bs);
    }

    /*!
     * Update the number of on bits
     */
    void updateNumOnBits();

    static BitSet null;
};

const Uint8 set_on_lookup[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
const Uint8 set_off_lookup[8] = {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE};

inline bool BitSet::get(Uint32 i) const
{
    if (i >= num_bits) {
        return false;
    }
    // i >> 3 equal to i / 8
    // i & 7 equal to i % 8
    return (data[i >> 3] & set_on_lookup[i & 7]) != 0;
}

inline void BitSet::set(Uint32 i, bool on)
{
    if (i >= num_bits) {
        return;
    }

    Uint8 *d = data + (i >> 3);
    num_on -= std::popcount(*d);
    if (on) {
        *d |= set_on_lookup[i & 7];
    } else {
        *d &= set_off_lookup[i & 7];
    }
    num_on += std::popcount(*d);
}
}

#endif
