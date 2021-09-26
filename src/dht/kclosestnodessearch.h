/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTKCLOSESTNODESSEARCH_H
#define DHTKCLOSESTNODESSEARCH_H

#include "kbucket.h"
#include "key.h"
#include <map>

namespace dht
{
class PackedNodeContainer;

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Class used to store the search results during a K closests nodes search
 * Note: we use a std::map because of lack of functionality in QMap
 */
class KClosestNodesSearch
{
    dht::Key key;
    std::map<dht::Key, KBucketEntry> emap;
    bt::Uint32 max_entries;

public:
    /**
     * Constructor sets the key to compare with
     * @param key The key to compare with
     * @param max_entries The maximum number of entries can be in the map
     * @return
     */
    KClosestNodesSearch(const dht::Key &key, bt::Uint32 max_entries);
    virtual ~KClosestNodesSearch();

    typedef std::map<dht::Key, KBucketEntry>::iterator Itr;
    typedef std::map<dht::Key, KBucketEntry>::const_iterator CItr;

    Itr begin()
    {
        return emap.begin();
    }
    Itr end()
    {
        return emap.end();
    }

    CItr begin() const
    {
        return emap.begin();
    }
    CItr end() const
    {
        return emap.end();
    }

    /// Get the target key of the search3
    const dht::Key &getSearchTarget() const
    {
        return key;
    }

    /// Get the number of entries.
    bt::Uint32 getNumEntries() const
    {
        return emap.size();
    }

    /**
     * Try to insert an entry.
     * @param e The entry
     */
    void tryInsert(const KBucketEntry &e);

    /**
     * Pack the search results in a PackedNodeContainer.
     * @param cnt Place to store IPv6 nodes
     */
    void pack(PackedNodeContainer *cnt);
};

}

#endif
