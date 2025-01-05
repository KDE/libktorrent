/*
    SPDX-FileCopyrightText: 2025 Andrius Štikonas <andrius@stikonas.eu>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTINFOHASH_H
#define BTINFOHASH_H

#include "sha1hash.h"
#include "sha2hash.h"

#include <ktorrent_export.h>

class QString;

namespace bt
{
/**
 * @author Andrius Štikonas
 * @brief Stores InfoHash(es)
 *
 * This class keeps track of InfoHash v1 and v2.
 */
class KTORRENT_EXPORT InfoHash
{
protected:
    SHA1Hash v1;
    SHA2Hash v2;

public:
    InfoHash();
    InfoHash(SHA1Hash v1, SHA2Hash v2);
    InfoHash(const InfoHash &other);

    InfoHash &operator=(const InfoHash &other);
    bool operator!=(const InfoHash &other) const;
    bool operator==(const InfoHash &other) const;

    bool hasV1() const;
    bool hasV2() const;

    SHA1Hash getV1() const;
    SHA2Hash getV2() const;

    SHA1Hash truncated() const;
    QString toString() const;
    QString toURLString() const;

    /**
     * Function to support the use of SHA2Hash as QHash keys
     * @param key SHA2Hash used to compute a hash key
     * @return hash key
     */
    KTORRENT_EXPORT friend size_t qHash(const InfoHash &key, size_t seed) noexcept;
};

}

#endif
