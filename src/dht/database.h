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
#ifndef DHTDATABASE_H
#define DHTDATABASE_H

#include <qmap.h>
#include <qlist.h>
#include <net/address.h>
#include <util/ptrmap.h>
#include <util/constants.h>
#include <util/array.h>
#include "key.h"

namespace dht
{

	/**
	 * @author Fonic <https://github.com/fonic>
	 * Maximum allowed age of database items before expiration. This is not
	 * to be confused with DATABASE_EXPIRE_INTERVAL in dht.h which defines
	 * how often the database will be checked for expired items. A value of
	 * 30 * 60* 1000 is proposed by the reference implementation.
	 */
	const bt::Uint32 MAX_ITEM_AGE = 30 * 60 * 1000;

	/**
	 * @author Joris Guisson
	 *
	 * Item in the database, will keep track of an IP and port combination.
	 * As well as the time it was inserted.
	 */
	class DBItem
	{
	public:
		DBItem();
		DBItem(const net::Address & addr);
		DBItem(const DBItem & item);
		virtual ~DBItem();

		// See if the item is expired
		bool expired(bt::TimeStamp now) const;

		// Get the address of an item
		const net::Address & getAddress() const {return addr;}

		/**
		 * Pack this item into a buffer, the buffer needs to big enough to handle IPv6 addresses (so 16 + 2 (for the port))
		 * @param buf The buffer
		 * @return The number of bytes used
		 */
		bt::Uint32 pack(bt::Uint8* buf) const;

		DBItem & operator = (const DBItem & item);
	private:
		net::Address addr;
		bt::TimeStamp time_stamp;
	};

	typedef QList<DBItem> DBItemList;

	/**
	 * @author Joris Guisson
	 *
	 * Class where all the key value paires get stored.
	*/
	class Database
	{
	public:
		Database();
		virtual ~Database();

		/**
		 * Store an entry in the database
		 * @param key The key
		 * @param dbi The DBItem to store
		 */
		void store(const dht::Key & key, const DBItem & dbi);

		/**
		 * Get max_entries items from the database, which have
		 * the same key, items are taken randomly from the list.
		 * If the key is not present no items will be returned, if
		 * there are fewer then max_entries items for the key, all
		 * entries will be returned
		 * @param key The key to search for
		 * @param dbl The list to store the items in
		 * @param max_entries The maximum number entries
		 * @param ip_version Wanted IP version (4 or 6)
		 */
		void sample(const dht::Key & key, DBItemList & dbl, bt::Uint32 max_entries, bt::Uint32 ip_version);

		/**
		 * Expire all items older then 30 minutes
		 * @param now The time it is now
		 * (we pass this along so we only have to calculate it once)
		 */
		void expire(bt::TimeStamp now);

		/**
		 * Generate a write token, which will give peers write access to
		 * the DB.
		 * @param addr The address of the peer
		 * @return A Key
		 */
		dht::Key genToken(const net::Address & addr);

		/**
		 * Check if a received token is OK.
		 * @param token The token received
		 * @param addr The address of the peer
		 * @return true if the token was given to this peer, false other wise
		 */
		bool checkToken(const dht::Key & token, const net::Address & addr);

		// Test whether or not the DB contains a key
		bool contains(const dht::Key & key) const;

		// Insert an empty item (only if it isn't already in the DB)
		void insert(const dht::Key & key);

	private:
		bt::PtrMap<dht::Key, DBItemList> items;
		QMap<dht::Key, bt::TimeStamp> tokens;
	};

}

#endif // DHTDATABASE_H
