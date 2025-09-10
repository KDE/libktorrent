/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTBADPEERSLIST_H
#define BTBADPEERSLIST_H

#include <QStringList>
#include <interfaces/blocklistinterface.h>

namespace bt
{
/*!
 * \headerfile peer/badpeerslist.h
 * \brief Blocklist to keep track of bad peers.
 */
class BadPeersList : public BlockListInterface
{
public:
    BadPeersList();
    ~BadPeersList() override;

    [[nodiscard]] bool blocked(const net::Address &addr) const override;

    //! Add a bad peer to the list
    void addBadPeer(const QString &ip);

private:
    QStringList bad_peers;
};

}

#endif
