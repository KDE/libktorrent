// SPDX-FileCopyrightText: 2026 Jack Hill <jackhill3103@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef BTMERKLETREE_H
#define BTMERKLETREE_H

#include <span>
#include <vector>

#include <util/constants.h>
#include <util/sha2hash.h>

namespace bt
{
class MerkleTree
{
    MerkleTree(std::span<SHA2Hash> base_layer);

private:
    /*!
     * An array of all hashes in the tree.
     */
    std::vector<SHA2Hash> m_hashes;
};
}

#endif // BTMERKLETREE_H
