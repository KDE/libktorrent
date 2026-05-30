// SPDX-FileCopyrightText: 2026 Jack Hill <jackhill3103@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "merkletree.h"

namespace
{
bool IsPow2(bt::Uint32 x)
{
    return (x & (x - 1)) == 0;
}

bt::Uint32 NextPow2(bt::Uint32 x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

bt::Uint32 NumLeavesFromNumPieces(bt::Uint32 num_pieces)
{
    return NextPow2(num_pieces);
}

bt::Uint32 NumHashesFromNumLeaves(bt::Uint32 num_leaves)
{
    Q_ASSERT(IsPow2(num_leaves));
    // Sum of all previous powers of 2 + this one
    // A.K.A just set all previous bits to 1
    return ((num_leaves - 1) << 1) + 1;
}
}

namespace bt
{
MerkleTree::MerkleTree(std::span<SHA2Hash> base_layer)
    : m_hashes(NumHashesFromNumLeaves(NumLeavesFromNumPieces(base_layer.size())))
{
    m_hashes.assign(base_layer.cbegin(), base_layer.cend());
}
}
