/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kbuckettable.h"
#include "dht.h"
#include "nodelookup.h"
#include <QFile>
#include <bcodec/bdecoder.h>
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/file.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
KBucketTable::KBucketTable(const Key &our_id)
    : our_id(our_id)
{
}

KBucketTable::~KBucketTable()
{
}

void KBucketTable::insert(const dht::KBucketEntry &entry, dht::RPCServerInterface *srv)
{
    if (buckets.empty()) {
        KBucket::Ptr initial(new KBucket(srv, our_id));
        buckets.push_back(initial);
    }

    KBucketList::iterator kb = findBucket(entry.getID());

    // return if we can't find a bucket, should never happen'
    if (kb == buckets.end()) {
        Out(SYS_DHT | LOG_IMPORTANT) << "Unable to find bucket !" << endl;
        return;
    }

    // insert it into the bucket
    try {
        if ((*kb)->insert(entry)) {
            // Bucket needs to be split
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
            if (result.first->keyInRange(entry.getID()))
                result.first->insert(entry);
            else
                result.second->insert(entry);
        }
    } catch (const KBucket::UnableToSplit &) {
        // Can't split, so stop this
        Out(SYS_DHT | LOG_IMPORTANT) << "Unable to split buckets further !" << endl;
        return;
    }
}

int KBucketTable::numEntries() const
{
    int count = 0;
    for (const KBucket::Ptr &b : std::as_const(buckets)) {
        count += b->getNumEntries();
    }

    return count;
}

KBucketTable::KBucketList::iterator KBucketTable::findBucket(const dht::Key &id)
{
    for (KBucketList::iterator i = buckets.begin(); i != buckets.end(); ++i) {
        if ((*i)->keyInRange(id))
            return i;
    }

    return buckets.end();
}

void KBucketTable::refreshBuckets(DHT *dh_table)
{
    for (const KBucket::Ptr &b : std::as_const(buckets)) {
        if (b->needsToBeRefreshed()) {
            // the key needs to be the refreshed
            dht::Key m = dht::Key::mid(b->minKey(), b->maxKey());
            NodeLookup *nl = dh_table->refreshBucket(m, *b);
            if (nl)
                b->setRefreshTask(nl);
        }
    }
}

void KBucketTable::onTimeout(const net::Address &addr)
{
    for (const KBucket::Ptr &b : std::as_const(buckets)) {
        if (b->onTimeout(addr))
            return;
    }
}

void KBucketTable::loadTable(const QString &file, RPCServerInterface *srv)
{
    QFile fptr(file);
    if (!fptr.open(QIODevice::ReadOnly)) {
        Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << file << " : " << fptr.errorString() << endl;
        return;
    }

    try {
        QByteArray data = fptr.readAll();
        bt::BDecoder dec(data, false, 0);

        const std::unique_ptr<BListNode> bucket_list = dec.decodeList();
        if (!bucket_list)
            return;

        for (bt::Uint32 i = 0; i < bucket_list->getNumChildren(); i++) {
            BDictNode *dict = bucket_list->getDict(i);
            if (!dict)
                continue;

            KBucket::Ptr bucket(new KBucket(srv, our_id));
            bucket->load(dict);
            buckets.push_back(bucket);
        }
    } catch (bt::Error &e) {
        Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Failed to load bucket table: " << e.toString() << endl;
    }
}

void KBucketTable::saveTable(const QString &file)
{
    bt::File fptr;
    if (!fptr.open(file, u"wb"_s)) {
        Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Cannot open file " << file << " : " << fptr.errorString() << endl;
        return;
    }

    BEncoder enc(&fptr);

    try {
        enc.beginList();
        for (const KBucket::Ptr &b : std::as_const(buckets)) {
            b->save(enc);
        }
        enc.end();
    } catch (bt::Error &err) {
        Out(SYS_DHT | LOG_IMPORTANT) << "DHT: Failed to save table to " << file << " : " << err.toString() << endl;
    }
}

void KBucketTable::findKClosestNodes(KClosestNodesSearch &kns) const
{
    for (const KBucket::Ptr &b : std::as_const(buckets)) {
        b->findKClosestNodes(kns);
    }
}

}
