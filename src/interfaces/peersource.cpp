/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "peersource.h"

namespace bt
{
PeerSource::PeerSource()
{
}

PeerSource::~PeerSource()
{
}

void PeerSource::completed()
{
}

void PeerSource::manualUpdate()
{
}

void PeerSource::aboutToBeDestroyed()
{
}

void PeerSource::addPeer(const net::Address &addr, bool local)
{
    peers.append(qMakePair(addr, local));
}

bool PeerSource::takePeer(net::Address &addr, bool &local)
{
    if (peers.count() > 0) {
        addr = peers.front().first;
        local = peers.front().second;
        peers.pop_front();
        return true;
    }
    return false;
}

}

#include "moc_peersource.cpp"
