/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTACCESSMANAGER_H
#define BTACCESSMANAGER_H

#include <QStringList>
#include <ktorrent_export.h>
#include <net/address.h>

namespace bt
{
class BlockListInterface;
class BadPeersList;

/*!
    \author Joris Guisson

    \brief Determines whether we allow an IP to connect to us.

    It uses blocklists to do this. Blocklists should register with this class.
    By default it has one blocklist, the banned peers list.
*/
class KTORRENT_EXPORT AccessManager
{
    AccessManager();

public:
    virtual ~AccessManager();

    //! Get the singleton instance
    static AccessManager &instance();

    //! Add a blocklist (AccessManager takes ownership unless list is explicitly remove with removeBlockList)
    void addBlockList(BlockListInterface *bl);

    //! Remove a blocklist
    void removeBlockList(BlockListInterface *bl);

    //! Are we allowed to have a connection with a peer
    [[nodiscard]] bool allowed(const net::Address &addr) const;

    //! Ban a peer (i.e. add it to the banned list)
    void banPeer(const QString &addr);

    //! Add an external IP throuch which we are reacheable
    void addExternalIP(const QString &addr);

private:
    [[nodiscard]] bool isOurOwnAddress(const net::Address &addr) const;

private:
    QList<BlockListInterface *> blocklists;
    BadPeersList *banned;
    QStringList external_addresses;
};

}

#endif
