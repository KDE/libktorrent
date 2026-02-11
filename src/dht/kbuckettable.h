/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_KBUCKETTABLE_H
#define DHT_KBUCKETTABLE_H

#include <dht/kbucket.h>
#include <list>

namespace dht
{
/*!
 * \brief Holds a table of buckets.
 */
class KBucketTable
{
public:
    KBucketTable(const Key &our_id);
    virtual ~KBucketTable();

    //! Insert a KBucketEntry into the table
    void insert(const KBucketEntry &entry, RPCServerInterface *srv);

    //! Get the number of entries
    [[nodiscard]] int numEntries() const;

    //! Refresh the buckets
    void refreshBuckets(DHT *dh_table);

    //! Timeout happened
    void onTimeout(const net::Address &addr);

    //! Load the table from a file
    void loadTable(const QString &file, dht::RPCServerInterface *srv);

    //! Save table to a file
    void saveTable(const QString &file);

    //! Find the K closest nodes
    void findKClosestNodes(KClosestNodesSearch &kns) const;

private:
    using KBucketList = std::list<std::unique_ptr<KBucket>>;
    inline KBucketList::iterator findBucket(const dht::Key &id);

private:
    Key our_id;
    KBucketList buckets;
};

}

#endif // DHT_KBUCKETTABLE_H
