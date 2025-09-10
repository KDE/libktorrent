/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

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

/*!
 * \headerfile dht/kclosestnodessearch.h
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * \brief Stores the search results during a K closests nodes search.
 *
 * Note: we use a std::map because of lack of functionality in QMap
 */
class KClosestNodesSearch
{
    dht::Key key;
    std::map<dht::Key, KBucketEntry> emap;
    bt::Uint32 max_entries;

public:
    /*!
     * Constructor sets the key to compare with
     * \param key The key to compare with
     * \param max_entries The maximum number of entries can be in the map
     */
    KClosestNodesSearch(const dht::Key &key, bt::Uint32 max_entries);
    virtual ~KClosestNodesSearch();

    using Itr = std::map<dht::Key, KBucketEntry>::iterator;
    using CItr = std::map<dht::Key, KBucketEntry>::const_iterator;

    Itr begin()
    {
        return emap.begin();
    }
    Itr end()
    {
        return emap.end();
    }

    [[nodiscard]] CItr begin() const
    {
        return emap.begin();
    }
    [[nodiscard]] CItr end() const
    {
        return emap.end();
    }

    //! Get the target key of the search3
    [[nodiscard]] const dht::Key &getSearchTarget() const
    {
        return key;
    }

    //! Get the number of entries.
    [[nodiscard]] bt::Uint32 getNumEntries() const
    {
        return emap.size();
    }

    /*!
     * Try to insert an entry.
     * \param e The entry
     */
    void tryInsert(const KBucketEntry &e);

    /*!
     * Pack the search results in a PackedNodeContainer.
     * \param cnt Place to store IPv6 nodes
     */
    void pack(PackedNodeContainer *cnt);
};

}

#endif
