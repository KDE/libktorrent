/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_KBUCKETENTRY_H
#define DHT_KBUCKETENTRY_H

#include <dht/key.h>
#include <net/address.h>
#include <set>

namespace dht
{
/*!
 * \author Joris Guisson
 *
 * \brief Entry in a KBucket, it basically contains an ip_address of a node, the udp port of the node and a node_id.
 */
class KBucketEntry
{
public:
    /*!
     * Constructor, sets everything to 0.
     */
    KBucketEntry();

    /*!
     * Constructor, set the ip, port and key
     * \param addr socket address
     * \param id ID of node
     */
    KBucketEntry(const net::Address &addr, const Key &id);

    /*!
     * Copy constructor.
     * \param other KBucketEntry to copy
     */
    KBucketEntry(const KBucketEntry &other);

    //! Destructor
    virtual ~KBucketEntry();

    /*!
     * Assignment operator.
     * \param other Node to copy
     * \return this KBucketEntry
     */
    KBucketEntry &operator=(const KBucketEntry &other);

    //! Equality operator
    bool operator==(const KBucketEntry &entry) const;

    //! Get the socket address of the node
    const net::Address &getAddress() const
    {
        return addr;
    }

    //! Get it's ID
    const Key &getID() const
    {
        return node_id;
    }

    //! Is this node a good node
    bool isGood() const;

    //! Is this node questionable (haven't heard from it in the last 15 minutes)
    bool isQuestionable() const;

    //! Is it a bad node. (Hasn't responded to a query
    bool isBad() const;

    //! Signal the entry that the peer has responded
    void hasResponded();

    //! A request timed out
    void requestTimeout()
    {
        failed_queries++;
    }

    //! The entry has been pinged because it is questionable
    void onPingQuestionable()
    {
        questionable_pings++;
    }

    //! The null entry
    static KBucketEntry null;

    //! < operator
    bool operator<(const KBucketEntry &entry) const;

private:
    net::Address addr;
    Key node_id;
    bt::TimeStamp last_responded;
    bt::Uint32 failed_queries;
    bt::Uint32 questionable_pings;
};

/*!
 * \brief Convenience wrapper around a std::set of KBucketEntry.
 */
class KBucketEntrySet : public std::set<KBucketEntry>
{
public:
    KBucketEntrySet()
    {
    }
    virtual ~KBucketEntrySet()
    {
    }

    bool contains(const KBucketEntry &entry) const
    {
        return find(entry) != end();
    }
};

}

#endif // DHT_KBUCKETENTRY_H
