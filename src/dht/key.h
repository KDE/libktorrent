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
#ifndef DHTKEY_H
#define DHTKEY_H

#include <QByteArray>
#include <util/sha1hash.h>
#include <ktorrent_export.h>

namespace dht
{

	/**
	 * @author Joris Guisson
	 * @brief Key in the distributed hash table
	 *
	 * Key's in the distributed hash table are just SHA-1 hashes.
	 * Key provides all necesarry operators to be used as a value.
	 */
	class KTORRENT_EXPORT Key : public bt::SHA1Hash
	{
	public:
		/**
		 * Constructor, sets key to 0.
		 */
		Key();

		/**
		 * Copy constructor. Seeing that Key doesn't add any data
		 * we just pass a SHA1Hash, Key's are automatically covered by this
		 * @param k Hash to copy
		 */
		Key(const bt::SHA1Hash & k);

		/**
		 * Make a key out of a bytearray
		 * @param ba The QByteArray
		 */
		Key(const QByteArray & ba);

		/**
		 * Make a key out of a 20 byte array.
		 * @param d The array
		 */
		Key(const bt::Uint8* d);

		// Destructor.
		virtual ~Key();

		/**
		 * Create a random key.
		 * @return A random Key
		 */
		static Key random();

		// Get the minimum key (all zeros)
		static Key min();

		// Get the maximum key (all FF)
		static Key max();

		/**
		 * Equality operator.
		 * @param other The key to compare
		 * @return true if this key is equal to other
		 */
		bool operator == (const Key & other) const;

		/**
		 * Inequality operator.
		 * @param other The key to compare
		 * @return true if this key is not equal to other
		 */
		bool operator != (const Key & other) const;

		/**
		 * Smaller then operator.
		 * @param other The key to compare
		 * @return rue if this key is smaller then other
		 */
		bool operator < (const Key & other) const;

		/**
		 * Smaller then or equal operator.
		 * @param other The key to compare
		 * @return rue if this key is smaller then or equal to other
		 */
		bool operator <= (const Key & other) const;

		/**
		 * Greater then operator.
		 * @param other The key to compare
		 * @return rue if this key is greater then other
		 */
		bool operator > (const Key & other) const;

		/**
		 * Greater then or equal operator.
		 * @param other The key to compare
		 * @return rue if this key is greater then or equal to other
		 */
		bool operator >= (const Key & other) const;

		/**
		 * Divide by a number operator
		 */
		Key operator / (int value) const;

		/**
		 * Addition for keys
		 * @param a The first key
		 * @param b The second key
		 */
		friend KTORRENT_EXPORT Key operator + (const Key & a, const Key & b);

		/**
		 * Subtraction for keys
		 * @param a The first key
		 * @param b The second key
		 */
		friend KTORRENT_EXPORT Key operator - (const Key & a, const Key & b);

		/**
		 * Addition for key and a value
		 * @param a The key
		 * @param b The value
		 */
		friend KTORRENT_EXPORT Key operator + (const Key & a, bt::Uint8 value);

		/**
		 * The distance of two keys is the keys xor together.
		 * @param a The first key
		 * @param b The second key
		 * @return a xor b
		 */
		static Key distance(const Key & a, const Key & b);

		/**
		 * Calculate the middle between two keys.
		 * @param a The first key
		 * @param b The second key
		 * @return The middle
		 */
		static Key mid(const Key & a, const Key & b);
	};

}

#endif // DHTKEY_H
