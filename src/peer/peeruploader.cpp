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
#include "peeruploader.h"
#include <set>
#include <util/log.h>
#include <util/functions.h>
#include <util/sha1hash.h>
#include "peer.h"
#include <diskio/chunkmanager.h>
#include <torrent/torrent.h>


namespace bt
{

	PeerUploader::PeerUploader(Peer* peer) : peer(peer), uploaded(0)
	{}


	PeerUploader::~PeerUploader()
	{}

	void PeerUploader::addRequest(const Request & r)
	{
		requests.push_back(r);
	}
	
	void PeerUploader::removeRequest(const Request & r)
	{
		requests.removeAll(r);
	}
	
	Uint32 PeerUploader::handleRequests(ChunkManager & cman)
	{
		Uint32 ret = uploaded;
		uploaded = 0;
		
		// if we have choked the peer do not upload
		if (peer->areWeChoked())
			return ret;
		
		while (requests.size() > 0)
		{
			Request r = requests.front();
			
			Chunk* c = cman.getChunk(r.getIndex());	
			if (c && c->getStatus() == Chunk::ON_DISK)
			{
				if (!peer->sendChunk(r.getIndex(),r.getOffset(),r.getLength(),c))
				{
					if (peer->getStats().fast_extensions)
						peer->sendReject(r);
				}
			}
			else
			{
				// remove requests we can't satisfy
				Out(SYS_CON|LOG_DEBUG) << "Cannot satisfy request" << endl;
				if (peer->getStats().fast_extensions)
					peer->sendReject(r);
			}
			
			requests.pop_front();
		}
		
		return ret;
	}
	
	void PeerUploader::clearAllRequests()
	{
		peer->clearPendingPieceUploads();
		if (peer->getStats().fast_extensions)
		{
			// reject all requests 
			// if the peer supports fast extensions, 
			// choke doesn't mean reject all
			foreach (const Request & r,requests)
				peer->sendReject(r);
		}
		requests.clear();
	}
		
	Uint32 PeerUploader::getNumRequests() const
	{
		return requests.size();
	}
}
