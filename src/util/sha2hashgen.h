/*
    SPDX-FileCopyrightText: 2024 Andrius Štikonas <andrius@stikonas.eu>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSHA2HASHGEN_H
#define BTSHA2HASHGEN_H

#include "constants.h"
#include "sha2hash.h"
#include <ktorrent_export.h>

class QCryptographicHash;

namespace bt
{
/**
 * @author Joris Guisson
 *
 * Generates a SHA2 has
 * - generate : all data is present from the start
 * - start, update and end : data can be delivered in chunks
 *
 * Mixing the 2, is not a good idea
 */
class KTORRENT_EXPORT SHA2HashGen
{
public:
    SHA2HashGen();
    ~SHA2HashGen();

    /**
     * Generate a hash from a bunch of data.
     * @param data The data
     * @return The SHA2 hash
     */
    SHA2Hash generate(QByteArrayView data);

    /**
     * Generate a hash from a bunch of data.
     * @param data The data
     * @param len The length
     * @return The SHA2 hash
     */
    SHA2Hash generate(const Uint8 *data, Uint32 len);

    /**
     * Start SHA2 hash generation in chunks.
     */
    void start();

    /**
     * Update the hash.
     * @param data The data
     */
    void update(QByteArrayView data);

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
    SHA2Hash get() const;

private:
    QCryptographicHash *h;
    Uint8 result[32];
};

}

#endif
