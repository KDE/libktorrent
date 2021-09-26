/*
    SPDX-FileCopyrightText: 2008 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "badpeerslist.h"
#include <net/address.h>

namespace bt
{
BadPeersList::BadPeersList()
{
}

BadPeersList::~BadPeersList()
{
}

bool BadPeersList::blocked(const net::Address &addr) const
{
    return bad_peers.contains(addr.toString());
}

void BadPeersList::addBadPeer(const QString &ip)
{
    bad_peers << ip;
}

}
