/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTKBUCKET_H
#define DHTKBUCKET_H

#include "kbucketentry.h"
#include "key.h"
#include "rpccall.h"
#include <QList>
#include <net/address.h>
#include <set>
#include <util/constants.h>

namespace bt
{
class BEncoder;
}

namespace dht
{
class RPCServerInterface;
class KClosestNodesSearch;
class Task;

const bt::Uint32 K = 20;
const bt::Uint32 BUCKET_MAGIC_NUMBER = 0xB0C4B0C4;
const bt::Uint32 BUCKET_REFRESH_INTERVAL = 15 * 60 * 1000;

/*!
 * \author Joris Guisson
 *
 * A KBucket is just a list of KBucketEntry objects.
 * The list is sorted by time last seen :
 * The first element is the least recently seen, the last
 * the most recently seen.
 */
class KBucket : public RPCCallListener
{
    Q_OBJECT

public:
    KBucket(RPCServerInterface *srv, const Key &our_id);
    KBucket(const dht::Key &min_key, const dht::Key &max_key, RPCServerInterface *srv, const Key &our_id);
    ~KBucket() override;

    typedef std::unique_ptr<KBucket> Ptr;

    //! Get the min key
    const dht::Key &minKey() const
    {
        return min_key;
    }

    //! Get the max key
    const dht::Key &maxKey() const
    {
        return max_key;
    }

    //! Does the key k lies in in the range of this bucket
    bool keyInRange(const dht::Key &k) const;

    //! Are we allowed to split
    bool splitAllowed() const;

    class UnableToSplit
    {
    };

    /*!
     * Split the bucket in two new buckets, each containing half the range of the original one.
     * \return A pair of KBucket's
     * \throw UnableToSplit if something goes wrong
     */
    std::pair<KBucket::Ptr, KBucket::Ptr> split() noexcept(false);

    /*!
     * Inserts an entry into the bucket.
     * \param entry The entry to insert
     * \return true If the bucket needs to be split, false otherwise
     */
    bool insert(const KBucketEntry &entry);

    //! Get the least recently seen node
    const KBucketEntry &leastRecentlySeen() const
    {
        return entries[0];
    }

    //! Get the number of entries
    bt::Uint32 getNumEntries() const
    {
        return entries.count();
    }

    //! See if this bucket contains an entry
    bool contains(const KBucketEntry &entry) const;

    /*!
     * Find the K closest entries to a key and store them in the KClosestNodesSearch
     * object.
     * \param kns The object to store the search results
     */
    void findKClosestNodes(KClosestNodesSearch &kns) const;

    /*!
     * A peer failed to respond
     * \param addr Address of the peer
     */
    bool onTimeout(const net::Address &addr);

    //! Check if the bucket needs to be refreshed
    bool needsToBeRefreshed() const;

    //! save the bucket to a file
    void save(bt::BEncoder &enc);

    //! Load the bucket from a file
    void load(bt::BDictNode *dict);

    //! Update the refresh timer of the bucket
    void updateRefreshTimer();

    //! Set the refresh task
    void setRefreshTask(Task *t);

private:
    void onResponse(RPCCall *c, RPCMsg::Ptr rsp) override;
    void onTimeout(RPCCall *c) override;
    void pingQuestionable(const KBucketEntry &replacement_entry);
    bool replaceBadEntry(const KBucketEntry &entry);

private Q_SLOTS:
    void onFinished(dht::Task *t);

private:
    dht::Key min_key, max_key;
    QList<KBucketEntry> entries, pending_entries;
    RPCServerInterface *srv;
    Key our_id;
    QMap<RPCCall *, KBucketEntry> pending_entries_busy_pinging;
    mutable bt::TimeStamp last_modified;
    Task *refresh_task;
};
}

template<class T>
inline size_t qHash(const T &e, size_t seed = 0) noexcept
{
    return e.hash(seed);
}

#endif
