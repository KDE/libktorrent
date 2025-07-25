/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTNODE_H
#define DHTNODE_H

#include "kbucket.h"
#include "key.h"
#include <QObject>

#include <memory>

namespace dht
{
class DHT;
class RPCMsg;
class RPCServer;
class KClosestNodesSearch;

const bt::Uint32 WANT_IPV4 = 1;
const bt::Uint32 WANT_IPV6 = 2;
const bt::Uint32 WANT_BOTH = WANT_IPV4 | WANT_IPV6;

/*!
 * \author Joris Guisson
 *
 * A Node represents us in the kademlia network. It contains
 * our id and 160 KBucket's.
 * A KBucketEntry is in node i, when the difference between our id and
 * the KBucketEntry's id is between 2 to the power i and 2 to the power i+1.
 */
class Node : public QObject
{
    Q_OBJECT
public:
    Node(RPCServer *srv, const QString &key_file);
    ~Node() override;

    /*!
     * An RPC message was received, the node must now update
     * the right bucket.
     * \param dh_table The DHT
     * \param msg The message
     */
    void received(DHT *dh_table, const RPCMsg &msg);

    //! Get our own ID
    const dht::Key &getOurID() const
    {
        return our_id;
    }

    /*!
     * Find the K closest entries to a key and store them in the KClosestNodesSearch
     * object.
     * \param kns The object to storre the search results
     * \param want Which protocol(s) are wanted
     */
    void findKClosestNodes(KClosestNodesSearch &kns, bt::Uint32 want);

    /*!
     * Increase the failed queries count of the bucket entry we sent the message to
     */
    void onTimeout(RPCMsg::Ptr msg);

    //! Check if a buckets needs to be refreshed, and refresh if necesarry
    void refreshBuckets(DHT *dh_table);

    //! Save the routing table to a file
    void saveTable(const QString &file);

    //! Load the routing table from a file
    void loadTable(const QString &file);

    //! Get the number of entries in the routing table
    bt::Uint32 getNumEntriesInRoutingTable() const
    {
        return num_entries;
    }

private:
    class Private;
    std::unique_ptr<Private> d;
    dht::Key our_id;
    bt::Uint32 num_entries;
};

}

#endif
