/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "chunkcounter.h"
#include <util/bitset.h>

namespace bt
{
ChunkCounter::ChunkCounter(Uint32 num_chunks)
    : cnt(num_chunks)
{
    // fill with 0
    cnt.fill(0);
}

ChunkCounter::~ChunkCounter()
{
}

void ChunkCounter::reset()
{
    cnt.fill(0);
}

void ChunkCounter::incBitSet(const BitSet &bs)
{
    for (Uint32 i = 0; i < cnt.size(); i++) {
        if (bs.get(i))
            cnt[i]++;
    }
}

void ChunkCounter::decBitSet(const BitSet &bs)
{
    for (Uint32 i = 0; i < cnt.size(); i++) {
        if (bs.get(i))
            dec(i);
    }
}

void ChunkCounter::inc(Uint32 idx)
{
    if (idx < cnt.size())
        cnt[idx]++;
}

void ChunkCounter::dec(Uint32 idx)
{
    if (idx < cnt.size() && cnt[idx] > 0)
        cnt[idx]--;
}

Uint32 ChunkCounter::get(Uint32 idx) const
{
    if (idx < cnt.size())
        return cnt[idx];
    else
        return 0;
}

}
