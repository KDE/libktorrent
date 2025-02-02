/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSHA1HASH_H
#define BTSHA1HASH_H

#include "constants.h"
#include <QByteArray>
#include <ktorrent_export.h>

class QString;

namespace bt
{
class Log;

/**
 * @author Joris Guisson
 * @brief Stores a SHA1 hash
 *
 * This class keeps track of a SHA1 hash. A SHA1 hash is a 20 byte
 * array of bytes.
 */
class KTORRENT_EXPORT SHA1Hash
{
protected:
    Uint32 hash[5];

public:
    /**
     * Constructor, sets every byte in the hash to 0.
     */
    SHA1Hash();

    /**
     * Copy constructor.
     * @param other Hash to copy
     */
    SHA1Hash(const SHA1Hash &other);

    /**
     * Directly set the hash data.
     * @param h The hash data must be 20 bytes large
     */
    SHA1Hash(QByteArrayView h);

    /**
     * Directly set the hash data.
     * @param h The hash data must be 20 bytes large
     */
    SHA1Hash(const Uint8 *h);

    /**
     * Destructor.
     */
    virtual ~SHA1Hash();

    /// Get the idx'th byte of the hash.
    Uint8 operator[](const Uint32 idx) const
    {
        return idx < 20 ? hash[idx] : 0;
    }

    /**
     * Assignment operator.
     * @param other Hash to copy
     */
    SHA1Hash &operator=(const SHA1Hash &other);

    /**
     * Test whether another hash is equal to this one.
     * @param other The other hash
     * @return true if equal, false otherwise
     */
    bool operator==(const SHA1Hash &other) const;

    /**
     * Test whether another hash is not equal to this one.
     * @param other The other hash
     * @return true if not equal, false otherwise
     */
    bool operator!=(const SHA1Hash &other) const
    {
        return !operator==(other);
    }

    /**
     * Generate an SHA1 hash from a bunch of data.
     * @param data The data
     * @return The generated SHA1 hash
     */
    static SHA1Hash generate(QByteArrayView data);

    /**
     * Generate an SHA1 hash from a bunch of data.
     * @param data The data
     * @param len Size in bytes of data
     * @return The generated SHA1 hash
     */
    static SHA1Hash generate(const Uint8 *data, Uint32 len);

    /**
     * Convert the hash to a printable string.
     * @return The string
     */
    QString toString() const;

    /**
     * Convert the hash to a string, usable in http get requests.
     * @return The string
     */
    QString toURLString() const;

    /**
     * Directly get pointer to the data.
     * @return The data
     */
    const Uint8 *getData() const
    {
        return (Uint8 *)hash;
    }

    /**
     * Function to print a SHA1Hash to the Log.
     * @param out The Log
     * @param h The hash
     * @return out
     */
    KTORRENT_EXPORT friend Log &operator<<(Log &out, const SHA1Hash &h);

    /**
     * XOR two SHA1Hashes
     * @param a The first hash
     * @param b The second
     * @return a xor b
     */
    KTORRENT_EXPORT friend SHA1Hash operator^(const SHA1Hash &a, const SHA1Hash &b);

    /**
     * Function to compare 2 hashes
     * @param a The first hash
     * @param h The second hash
     * @return whether a is smaller then b
     */
    KTORRENT_EXPORT friend bool operator<(const SHA1Hash &a, const SHA1Hash &b);

    /**
     * Function to support the use of SHA1Hash as QHash keys
     * @param key SHA1Hash used to compute a hash key
     * @return hash key
     */
    KTORRENT_EXPORT friend size_t qHash(const SHA1Hash &key, size_t seed) noexcept;

    /**
     * Convert the hash to a byte array.
     */
    QByteArray toByteArray() const;
};

}

#endif
