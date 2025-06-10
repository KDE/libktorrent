/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTDATABASE_H
#define DHTDATABASE_H

#include "key.h"
#include <QList>
#include <QMap>
#include <net/address.h>
#include <util/array.h>
#include <util/constants.h>
#include <util/ptrmap.h>

namespace dht
{
//! Each item may only exist for 30 minutes
const bt::Uint32 MAX_ITEM_AGE = 30 * 60 * 1000;

/*!
 * \author Joris Guisson
 *
 * \brief Item in the database, keeps track of an IP and port combination as well as the time it was inserted.
 */
class DBItem
{
public:
    DBItem();
    DBItem(const net::Address &addr);
    DBItem(const DBItem &item);
    virtual ~DBItem();

    //! See if the item is expired
    bool expired(bt::TimeStamp now) const;

    //! Get the address of an item
    const net::Address &getAddress() const
    {
        return addr;
    }

    /*!
     * Pack this item into a buffer, the buffer needs to big enough to handle IPv6 addresses (so 16 + 2 (for the port))
     * \param buf The buffer
     * \return The number of bytes used
     */
    bt::Uint32 pack(bt::Uint8 *buf) const;

    DBItem &operator=(const DBItem &item);

private:
    net::Address addr;
    bt::TimeStamp time_stamp;
};

typedef QList<DBItem> DBItemList;

/*!
 * \author Joris Guisson
 *
 * \brief Database where all the key value pairs get stored.
 */
class Database
{
public:
    Database();
    virtual ~Database();

    /*!
     * Store an entry in the database
     * \param key The key
     * \param dbi The DBItem to store
     */
    void store(const dht::Key &key, const DBItem &dbi);

    /*!
     * Get max_entries items from the database, which have
     * the same key, items are taken randomly from the list.
     * If the key is not present no items will be returned, if
     * there are fewer then max_entries items for the key, all
     * entries will be returned
     * \param key The key to search for
     * \param dbl The list to store the items in
     * \param max_entries The maximum number entries
     * \param ip_version Wanted IP version (4 or 6)
     */
    void sample(const dht::Key &key, DBItemList &dbl, bt::Uint32 max_entries, bt::Uint32 ip_version);

    /*!
     * Expire all items older then 30 minutes
     * \param now The time it is now
     * (we pass this along so we only have to calculate it once)
     */
    void expire(bt::TimeStamp now);

    /*!
     * Generate a write token, which will give peers write access to
     * the DB.
     * \param addr The address of the peer
     * \return A QByteArray
     */
    QByteArray genToken(const net::Address &addr);

    /*!
     * Check if a received token is OK.
     * \param token The token received
     * \param addr The address of the peer
     * \return true if the token was given to this peer, false other wise
     */
    bool checkToken(const QByteArray &token, const net::Address &addr);

    //! Test whether or not the DB contains a key
    bool contains(const dht::Key &key) const;

    //! Insert an empty item (only if it isn't already in the DB)
    void insert(const dht::Key &key);

private:
    bt::PtrMap<dht::Key, DBItemList> items;
    QMap<QByteArray, bt::TimeStamp> tokens;
};

}

#endif
