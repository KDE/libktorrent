/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "accessmanager.h"

#include <interfaces/blocklistinterface.h>
#include <net/address.h>
#include <peer/badpeerslist.h>
#include <torrent/server.h>
#include <tracker/tracker.h>

namespace bt
{
AccessManager::AccessManager()
{
    banned = new BadPeersList();
    addBlockList(banned);
}

AccessManager::~AccessManager()
{
    qDeleteAll(blocklists);
}

AccessManager &AccessManager::instance()
{
    static AccessManager inst;
    return inst;
}

void AccessManager::addBlockList(BlockListInterface *bl)
{
    blocklists.append(bl);
}

void AccessManager::removeBlockList(BlockListInterface *bl)
{
    blocklists.removeAll(bl);
}

bool AccessManager::allowed(const net::Address &addr) const
{
    // Don't connect to the custom IP sent to the tracker, this should be our own'
    if (isOurOwnAddress(addr))
        return false;

    for (const BlockListInterface *bl : qAsConst(blocklists)) {
        if (bl->blocked(addr))
            return false;
    }
    return true;
}

void AccessManager::banPeer(const QString &addr)
{
    banned->addBadPeer(addr);
}

void AccessManager::addExternalIP(const QString &addr)
{
    external_addresses.append(addr);
}

bool AccessManager::isOurOwnAddress(const net::Address &addr) const
{
    Uint16 port = bt::Server::getPort();
    if (!Tracker::getCustomIP().isEmpty() && net::Address(Tracker::getCustomIP(), port) == addr)
        return true;

    for (const QString &ip : qAsConst(external_addresses)) {
        net::Address address(ip, port);
        if (address == addr)
            return true;
    }

    return false;
}

}
