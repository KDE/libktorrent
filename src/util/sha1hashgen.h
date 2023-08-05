/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSHA1HASHGEN_H
#define BTSHA1HASHGEN_H

#include "constants.h"
#include "sha1hash.h"
#include <ktorrent_export.h>

class QCryptographicHash;

namespace bt
{
/**
 * @author Joris Guisson
 *
 * Generates a SHA1 hash, code based on wikipedia's pseudocode
 * There are 2 ways to use this class :
 * - generate : all data is present from the start
 * - start, update and end : data can be delivered in chunks
 *
 * Mixing the 2, is not a good idea
 */
class KTORRENT_EXPORT SHA1HashGen
{
public:
    SHA1HashGen();
    ~SHA1HashGen();

    /**
     * Generate a hash from a bunch of data.
     * @param data The data
     * @param len The length
     * @return The SHA1 hash
     */
    SHA1Hash generate(const Uint8 *data, Uint32 len);

    /**
     * Start SHA1 hash generation in chunks.
     */
    void start();

    /**
     * Update the hash.
     * @param data The data
     * @param len Length of the data
     */
    void update(const Uint8 *data, Uint32 len);

    /**
     * All data has been delivered, calculate the final hash.
     * @return
     */
    void end();

    /**
     * Get the hash generated.
     */
    SHA1Hash get() const;

private:
    QCryptographicHash *h;
    Uint8 result[20];
};

}

#endif
