/*
    SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTVERSION_H
#define BTVERSION_H

#include <ktorrent_export.h>
#include <libktorrent_version.h>
#include <util/constants.h>

class QString;

namespace bt
{
enum VersionType {
    ALPHA,
    BETA,
    RELEASE_CANDIDATE,
    DEVEL,
    NORMAL,
};

/**
 * Set the client info. This information is used to create
 * the PeerID and the version string (used in HTTP announces for example).
 * @param name Name of the client
 * @param major Major version
 * @param minor Minor version
 * @param release Release version
 * @param type Which version
 * @param peer_id_code Peer ID code (2 letters identifying the client, KT for KTorrent)
 */
KTORRENT_EXPORT void SetClientInfo(const QString &name, int major, int minor, int release, VersionType type, const QString &peer_id_code);

/**
 * Set the client info. This information is used to create
 * the PeerID and the version string (used in HTTP announces for example).
 * @param name Name of the client
 * @param version Version
 * @param peer_id_code Peer ID code (2 letters identifying the client, KT for KTorrent)
 */
KTORRENT_EXPORT void SetClientInfo(const QString &name, const QString &version, const QString &peer_id_code);

/**
 * Get the PeerID prefix set by SetClientInfo
 * @return The PeerID prefix
 */
KTORRENT_EXPORT QString PeerIDPrefix();

/**
 * Get the current client version string
 */
KTORRENT_EXPORT QString GetVersionString();

/// Major version number of the ktorrent library
const Uint32 MAJOR = LIBKTORRENT_VERSION_MAJOR;
/// Minor version number of the ktorrent library
const Uint32 MINOR = LIBKTORRENT_VERSION_MINOR;
/// Version type of the ktorrent library
const VersionType VERSION_TYPE = NORMAL;
/// Release version number of the ktorrent library (only for normal releases)
const Uint32 RELEASE = LIBKTORRENT_VERSION_PATCH;
}

#endif
