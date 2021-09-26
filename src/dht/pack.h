/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTPACK_H
#define DHTPACK_H

#include "kbucket.h"

namespace dht
{
/**
 * Pack a KBucketEntry into a byte array.
 * If the array is not large enough, an error will be thrown
 * @param e The entry
 * @param ba The byte array
 * @param off The offset into the array
 */
void PackBucketEntry(const KBucketEntry &e, QByteArray &ba, bt::Uint32 off);

/**
 * Unpack a KBucketEntry from a byte array.
 * If a full entry cannot be read an error will be thrown.
 * @param ba The byte array
 * @param off The offset
 * @param ip_version The ip version (4 or 6)
 * @return The entry
 */
KBucketEntry UnpackBucketEntry(const QByteArray &ba, bt::Uint32 off, int ip_version);

}

#endif
