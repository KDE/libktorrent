/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCHUNKCOUNTER_H
#define BTCHUNKCOUNTER_H

#include <util/array.h>
#include <util/constants.h>

namespace bt
{
class BitSet;

/*!
 * \author Joris Guisson
 *
 * Class to keep track of how many peers have a chunk.
 */
class KTORRENT_EXPORT ChunkCounter
{
    Array<Uint32> cnt;

public:
    ChunkCounter(Uint32 num_chunks);
    virtual ~ChunkCounter();

    /*!
     * If a bit in the bitset is one, increment the corresponding counter.
     * \param bs The BitSet
     */
    void incBitSet(const BitSet &bs);

    /*!
     * If a bit in the bitset is one, decrement the corresponding counter.
     * \param bs The BitSet
     */
    void decBitSet(const BitSet &bs);

    /*!
     * Increment the counter for the idx'th chunk
     * \param idx Index of the chunk
     */
    void inc(Uint32 idx);

    /*!
     * Decrement the counter for the idx'th chunk
     * \param idx Index of the chunk
     */
    void dec(Uint32 idx);

    /*!
     * Get the counter for the idx'th chunk
     * \param idx Index of the chunk
     */
    Uint32 get(Uint32 idx) const;

    /*!
     * Reset all values to 0
     */
    void reset();

    //! Get the number of chunks there are
    Uint32 getNumChunks() const
    {
        return cnt.size();
    }
};

}

#endif
