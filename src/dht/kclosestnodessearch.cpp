/***************************************************************************
 *   Copyright (C) 2012 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
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
#include "kclosestnodessearch.h"
#include <util/functions.h>
#include "pack.h"
#include "packednodecontainer.h"

using namespace bt;

namespace dht
{
	typedef std::map<dht::Key, KBucketEntry>::iterator KNSitr;

	KClosestNodesSearch::KClosestNodesSearch(const dht::Key & key, Uint32 max_entries)
			: key(key), max_entries(max_entries)
	{
	}

	KClosestNodesSearch::~KClosestNodesSearch()
	{
	}

	void KClosestNodesSearch::tryInsert(const KBucketEntry & e)
	{
		// Calculate distance between key and e
		dht::Key d = dht::Key::distance(key, e.getID());

		if (emap.size() < max_entries)
		{
			// Room in the map so just insert
			emap.insert(std::make_pair(d, e));
		}
		else
		{
			// Now find the max distance. Seeing that the last element of the
			// map has also the biggest distance to key (std::map is sorted on
			// the distance) we just take the last
			const dht::Key & max = emap.rbegin()->first;
			if (d < max)
			{
				// Insert if d is smaller then max
				emap.insert(std::make_pair(d, e));
				// Erase the old max value
				emap.erase(max);
			}
		}
	}

	void KClosestNodesSearch::pack(PackedNodeContainer* cnt)
	{
		Uint32 j = 0;

		KNSitr i = emap.begin();
		while (i != emap.end())
		{
			const KBucketEntry & e = i->second;
			if (e.getAddress().ipVersion() == 4)
			{
				QByteArray d(26, 0);
				PackBucketEntry(i->second, d, 0);
				cnt->addNode(d);
			}
			else
			{
				QByteArray d(38, 0);
				PackBucketEntry(i->second, d, 0);
				cnt->addNode(d);
			}
			j++;
			i++;
		}
	}

}
