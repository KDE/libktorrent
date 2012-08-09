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
#include <QFile>
#include <util/log.h>
#include <util/file.h>
#include <util/error.h>
#include <bcodec/bencoder.h>
#include <bcodec/bdecoder.h>
#include <bcodec/bnode.h>
#include "nodelookup.h"
#include "dht.h"



using namespace bt;

namespace dht
{

	KBucketTable::KBucketTable(const Key & our_id) :
		our_id(our_id)
	{
	}

	KBucketTable::~KBucketTable()
	{
	}

	void KBucketTable::insert(const dht::KBucketEntry& entry, dht::RPCServerInterface* srv)
	{
		if(buckets.empty())
		{
			KBucket::Ptr initial(new KBucket(srv, our_id));
			buckets.push_back(initial);
		}


		KBucketList::iterator kb = findBucket(entry.getID());

		// return if we can't find a bucket, should never happen'
		if(kb == buckets.end())
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "Unable to find bucket !" << endl;
			return;
		}

		// insert it into the bucket
		try
		{
			if((*kb)->insert(entry))
			{
				// Bucket needs to be splitted
				std::pair<KBucket::Ptr, KBucket::Ptr> result = (*kb)->split();
/*
				Out(SYS_DHT | LOG_DEBUG) << "Splitting bucket " << (*kb)->minKey().toString() << "-" << (*kb)->maxKey().toString() << endl;
				Out(SYS_DHT | LOG_DEBUG) << "L: " << result.first->minKey().toString() << "-" << result.first->maxKey().toString() << endl;
				Out(SYS_DHT | LOG_DEBUG) << "L range: " << (result.first->maxKey() - result.first->minKey()).toString() << endl;
				Out(SYS_DHT | LOG_DEBUG) << "R: " << result.second->minKey().toString() << "-" << result.second->maxKey().toString() << endl;
				Out(SYS_DHT | LOG_DEBUG) << "R range: " << (result.second->maxKey() - result.second->minKey()).toString() << endl;
*/
				buckets.insert(kb, result.first);
				buckets.insert(kb, result.second);
				buckets.erase(kb);
				if(result.first->keyInRange(entry.getID()))
					result.first->insert(entry);
				else
					result.second->insert(entry);
			}
		}
		catch(const KBucket::UnableToSplit &)
		{
			// Can't split, so stop this
			Out(SYS_DHT | LOG_IMPORTANT) << "Unable to split buckets further !" << endl;
			return;
		}

	}

	int KBucketTable::numEntries() const
	{
		int count = 0;
		for(KBucketList::const_iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			count += (*i)->getNumEntries();
		}

		return count;
	}

	KBucketTable::KBucketList::iterator KBucketTable::findBucket(const dht::Key& id)
	{
		for(KBucketList::iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			if((*i)->keyInRange(id))
				return i;
		}

		return buckets.end();
	}

	void KBucketTable::refreshBuckets(DHT* dh_table)
	{
		for(KBucketList::iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			KBucket::Ptr b = *i;
			if(b->needsToBeRefreshed())
			{
				// the key needs to be the refreshed
				dht::Key m = dht::Key::mid(b->maxKey(), b->maxKey());
				NodeLookup* nl = dh_table->refreshBucket(m, *b);
				if(nl)
					b->setRefreshTask(nl);
			}
		}
	}

	void KBucketTable::onTimeout(const net::Address& addr)
	{
		for(KBucketList::iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			KBucket::Ptr b = *i;
			if(b->onTimeout(addr))
				return;
		}
	}

	void KBucketTable::loadTable(const QString& file, RPCServerInterface* srv)
	{
		QFile fptr(file);
		if(!fptr.open(QIODevice::ReadOnly))
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << file << " : " << fptr.errorString() << endl;
			return;
		}

		try
		{
			QByteArray data = fptr.readAll();
			bt::BDecoder dec(data, false, 0);

			QScopedPointer<BListNode> bucket_list(dec.decodeList());
			if(!bucket_list)
				return;

			for(bt::Uint32 i = 0; i < bucket_list->getNumChildren(); i++)
			{
				BDictNode* dict = bucket_list->getDict(i);
				if(!dict)
					continue;

				KBucket::Ptr bucket(new KBucket(srv, our_id));
				bucket->load(dict);
				buckets.push_back(bucket);
			}
		}
		catch(bt::Error & e)
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Failed to load bucket table: " << e.toString() << endl;
		}
	}

	void KBucketTable::saveTable(const QString& file)
	{
		bt::File fptr;
		if(!fptr.open(file, "wb"))
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << file << " : " << fptr.errorString() << endl;
			return;
		}

		BEncoder enc(&fptr);

		try
		{
			enc.beginList();
			for(KBucketList::iterator i = buckets.begin(); i != buckets.end(); i++)
			{
				KBucket::Ptr b = *i;
				b->save(enc);
			}
			enc.end();
		}
		catch(bt::Error & err)
		{
			Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Failed to save table to " << file << " : " << err.toString() << endl;
		}
	}

	void KBucketTable::findKClosestNodes(KClosestNodesSearch& kns)
	{
		for(KBucketList::iterator i = buckets.begin(); i != buckets.end(); i++)
		{
			KBucket::Ptr b = *i;
			b->findKClosestNodes(kns);
		}
	}

}
