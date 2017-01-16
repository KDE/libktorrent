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
#ifndef DHT_KBUCKETTABLE_H
#define DHT_KBUCKETTABLE_H

#include <list>
#include <dht/kbucket.h>

namespace dht
{

	/**
	 * Holds a table of buckets.
	 */
	class KBucketTable
	{
	public:
		KBucketTable(const Key & our_id);
		virtual ~KBucketTable();

		/**
		 * Insert a KBucketEntry into the table
		 */
		void insert(const KBucketEntry & entry, RPCServerInterface* srv);

		/**
		 * Get the number of entries
		 */
		int numEntries() const;

		/**
		 * Refresh the buckets
		 */
		void refreshBuckets(DHT* dh_table);

		/**
		 * Timeout happened
		 */
		void onTimeout(const net::Address & addr);

		/**
		 * Load the table from a file
		 */
		void loadTable(const QString& file, dht::RPCServerInterface* srv);

		/**
		 * Save table to a file
		 */
		void saveTable(const QString & file);

		/**
		 * Find the K closest nodes
		 */
		void findKClosestNodes(KClosestNodesSearch & kns);

	private:
		typedef std::list<KBucket::Ptr> KBucketList;
		KBucketList::iterator findBucket(const dht::Key & id);

	private:
		Key our_id;
		KBucketList buckets;
	};

}

#endif // DHT_KBUCKETTABLE_H
