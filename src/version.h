/***************************************************************************
 *   Copyright (C) 2007 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
 *   ivasic@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#ifndef BTVERSION_H
#define BTVERSION_H

#include <ktorrent_export.h>
#include <util/constants.h>

#define LIBKTORRENT_MAJOR 1
#define LIBKTORRENT_MINOR 3
#define LIBKTORRENT_RELEASE 0
#define LIBKTORRENT_VERSION ((LIBKTORRENT_MAJOR << 16) | (LIBKTORRENT_MINOR << 8) | LIBKTORRENT_RELEASE)

class QString;

namespace bt 
{
	enum VersionType
	{
		ALPHA,
		BETA,
		RELEASE_CANDIDATE,
		DEVEL,
		NORMAL
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
	KTORRENT_EXPORT void SetClientInfo(const QString & name,int major,int minor,int release,VersionType type,const QString & peer_id_code);

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
	const Uint32 MAJOR = LIBKTORRENT_MAJOR;
	/// Minor version number of the ktorrent library
	const Uint32 MINOR = LIBKTORRENT_MINOR;
	/// Version type of the ktorrent library
	const VersionType VERSION_TYPE = NORMAL;
	/// Release version number only applicable for betas, alphas and rc's of libktorrent
	const Uint32 BETA_ALPHA_RC_RELEASE = 0;
	/// Release version number of the ktorrent library (only for normal releases)
	const Uint32 RELEASE = LIBKTORRENT_RELEASE;
}

#endif
