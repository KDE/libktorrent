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

#include "kbuckettable.h"
#include <util/log.h>
#include <util/file.h>
#include <util/error.h>
#include "nodelookup.h"
#include "dht.h"

using namespace bt;

namespace dht
{
	/// Generate a random key which lies in a certain bucket
	static Key RandomKeyInBucket(Uint32 b, const Key & our_id)
	{
		// first generate a random one
		Key r = dht::Key::random();
		Uint8* data = (Uint8*)r.getData();

		// before we hit bit b, everything needs to be equal to our_id
		Uint8 nb = b / 8;
		for (Uint8 i = 0;i < nb;i++)
			data[i] = *(our_id.getData() + i);


		// copy all bits of ob, until we hit the bit which needs to be different
		Uint8 ob = *(our_id.getData() + nb);
		for (Uint8 j = 0;j < b % 8;j++)
		{
			if ((0x80 >> j) & ob)
				data[nb] |= (0x80 >> j);
			else
				data[nb] &= ~(0x80 >> j);
		}

		// if the bit b is on turn it off else turn it on
		if ((0x80 >> (b % 8)) & ob)
			data[nb] &= ~(0x80 >> (b % 8));
		else
			data[nb] |= (0x80 >> (b % 8));

		return Key(data);
	}

	KBucketTable::KBucketTable(const Key & our_id) :
			our_id(our_id)
	{
	}

	KBucketTable::~KBucketTable()
	{
	}

	void KBucketTable::insert(const dht::KBucketEntry& entry, dht::RPCServerInterface* srv)
	{
		Uint8 bit_on = findBucket(entry.getID());

		// return if bit_on is not good
		if (bit_on >= 160)
			return;

		// make the bucket if it doesn't exist
		QMap<bt::Uint8, KBucket::Ptr>::iterator i = buckets.find(bit_on);
		if (i == buckets.end())
			i = buckets.insert(bit_on, KBucket::Ptr(new KBucket(bit_on, srv, our_id)));

		// insert it into the bucket
		KBucket::Ptr kb = i.value();
		kb->insert(entry);
	}

	int KBucketTable::numEntries() const
	{
		int count = 0;
		for (QMap<bt::Uint8, KBucket::Ptr>::const_iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			count += i.value()->getNumEntries();
		}

		return count;
	}

	Uint8 KBucketTable::findBucket(const dht::Key& id)
	{
		// XOR our id and the sender's ID
		dht::Key d = dht::Key::distance(id, our_id);
		// now use the first on bit to determin which bucket it should go in

		Uint8 bit_on = 0xFF;
		for (Uint32 i = 0;i < 20;i++)
		{
			// get the byte
			Uint8 b = *(d.getData() + i);
			// no bit on in this byte so continue
			if (b == 0x00)
				continue;

			for (Uint8 j = 0;j < 8;j++)
			{
				if (b & (0x80 >> j))
				{
					// we have found the bit
					bit_on = (19 - i) * 8 + (7 - j);
					return bit_on;
				}
			}
		}
		return bit_on;
	}

	void KBucketTable::refreshBuckets(DHT* dh_table)
	{
		for (QMap<bt::Uint8, KBucket::Ptr>::iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			KBucket::Ptr b = i.value();
			if (b->needsToBeRefreshed())
			{
				// the key needs to be the refreshed
				NodeLookup* nl = dh_table->refreshBucket(RandomKeyInBucket(i.key(), our_id), *b);
				if (nl)
					b->setRefreshTask(nl);
			}
		}
	}
	
	void KBucketTable::onTimeout(const net::Address& addr)
	{
		for (QMap<bt::Uint8, KBucket::Ptr>::iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			KBucket::Ptr b = i.value();
			if (b->onTimeout(addr))
				return;
		}
	}

	void KBucketTable::loadTable(const QString& file, RPCServerInterface* srv)
	{
		bt::File fptr;
		if (!fptr.open(file, "rb"))
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << file << " : " << fptr.errorString() << endl;
			return;
		}
		
		while (!fptr.eof())
		{
			BucketHeader hdr;
			try
			{
				if (fptr.read(&hdr, sizeof(BucketHeader)) != sizeof(BucketHeader))
					return;
			}
			catch (bt::Error & err)
			{
				Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Failed to load table from " << file << " : " << err.toString() << endl;
				return;
			}
			
			// new IPv6 capable format uses the old magic number + 1
			if (hdr.magic != dht::BUCKET_MAGIC_NUMBER + 1 || hdr.num_entries > dht::K || hdr.index > 160)
				return;
			
			if (hdr.num_entries == 0)
				continue;
			
			Out(SYS_DHT | LOG_NOTICE) << "DHT: Loading bucket " << hdr.index << endl;
			KBucket::Ptr bucket(new KBucket(hdr.index, srv, our_id));
			bucket->load(fptr, hdr);
			buckets[hdr.index] = bucket; 
		}
	}

	void KBucketTable::saveTable(const QString& file)
	{
		bt::File fptr;
		if (!fptr.open(file, "wb"))
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << file << " : " << fptr.errorString() << endl;
			return;
		}
		
		try
		{
			for (QMap<bt::Uint8, KBucket::Ptr>::iterator i = buckets.begin(); i != buckets.end(); i++)
			{
				KBucket::Ptr b = i.value();
				b->save(fptr);
			}
		}
		catch (bt::Error & err)
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Failed to save table to " << file << " : " << err.toString() << endl;
		}
	}
	
	void KBucketTable::findKClosestNodes(KClosestNodesSearch& kns)
	{
		for (QMap<bt::Uint8, KBucket::Ptr>::iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			KBucket::Ptr b = i.value();
			b->findKClosestNodes(kns);
		}
	}

}
