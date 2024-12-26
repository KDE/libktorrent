/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "peerprotocolextension.h"
#include "peer.h"
#include <cstdio>

namespace bt
{
PeerProtocolExtension::PeerProtocolExtension(Uint32 id, Peer *peer)
    : id(id)
    , peer(peer)
{
}

PeerProtocolExtension::~PeerProtocolExtension()
{
}

void PeerProtocolExtension::sendPacket(const QByteArray &data)
{
    peer->sendExtProtMsg(id, data);
}

void PeerProtocolExtension::update()
{
}

void PeerProtocolExtension::changeID(Uint32 id)
{
    this->id = id;
}

}
