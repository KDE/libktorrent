/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include <util/functions.h>
#include "peerinterface.h"

namespace bt
{

	PeerInterface::PeerInterface(const PeerID & peer_id, Uint32 num_chunks) : peer_id(peer_id),pieces(num_chunks)
	{
		stats.interested = false;
		stats.am_interested = false;
		stats.choked = true;
		stats.interested = false;
		stats.am_interested = false;
		stats.download_rate = 0;
		stats.upload_rate = 0;
		stats.perc_of_file = 0;
		stats.snubbed = false;
		stats.dht_support = false;
		stats.fast_extensions = false;
		stats.extension_protocol = false;
		stats.bytes_downloaded = stats.bytes_uploaded = 0;
		stats.aca_score = 0.0;
		stats.has_upload_slot = false;
		stats.num_up_requests = stats.num_down_requests = 0;
		stats.encrypted = false;
		stats.local = false;
		stats.max_request_queue = 0;
		stats.time_choked = CurrentTime();
		stats.time_unchoked = 0;
		killed = false;
		paused = false;
	}


	PeerInterface::~PeerInterface()
	{}


}
