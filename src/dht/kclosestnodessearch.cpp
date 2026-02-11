/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "kclosestnodessearch.h"
#include "pack.h"
#include "packednodecontainer.h"
#include <util/functions.h>

using namespace bt;

namespace dht
{
using KNSitr = std::map<dht::Key, KBucketEntry>::iterator;

KClosestNodesSearch::KClosestNodesSearch(const dht::Key &key, Uint32 max_entries)
    : key(key)
    , max_entries(max_entries)
{
}

KClosestNodesSearch::~KClosestNodesSearch()
{
}

void KClosestNodesSearch::tryInsert(const KBucketEntry &e)
{
    // calculate distance between key and e
    dht::Key d = dht::Key::distance(key, e.getID());

    if (emap.size() < max_entries) {
        // room in the map so just insert
        emap.insert(std::make_pair(d, e));
    } else {
        // now find the max distance
        // seeing that the last element of the map has also
        // the biggest distance to key (std::map is sorted on the distance)
        // we just take the last
        const dht::Key &max = emap.rbegin()->first;
        if (d < max) {
            // insert if d is smaller then max
            emap.insert(std::make_pair(d, e));
            // erase the old max value
            emap.erase(max);
        }
    }
}

void KClosestNodesSearch::pack(PackedNodeContainer *cnt)
{
    Uint32 j = 0;

    KNSitr i = emap.begin();
    while (i != emap.end()) {
        const KBucketEntry &e = i->second;
        if (e.getAddress().ipVersion() == 4) {
            QByteArray d(26, 0);
            PackBucketEntry(i->second, d, 0);
            cnt->addNode(d);
        } else {
            QByteArray d(38, 0);
            PackBucketEntry(i->second, d, 0);
            cnt->addNode(d);
        }
        j++;
        ++i;
    }
}

}
