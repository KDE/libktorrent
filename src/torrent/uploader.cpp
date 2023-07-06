/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "uploader.h"
#include <diskio/chunkmanager.h>
#include <download/request.h>
#include <peer/peer.h>
#include <peer/peermanager.h>
#include <peer/peeruploader.h>
#include <util/log.h>

namespace bt
{
Uploader::Uploader(ChunkManager &cman, PeerManager &pman)
    : cman(cman)
    , pman(pman)
    , uploaded(0)
{
}

Uploader::~Uploader()
{
}

void Uploader::visit(const bt::Peer::Ptr p)
{
    PeerUploader *pu = p->getPeerUploader();
    uploaded += pu->handleRequests(cman);
}

void Uploader::update()
{
    pman.visit(*this);
}

Uint32 Uploader::uploadRate() const
{
    return pman.uploadRate();
}

}

#include "moc_uploader.cpp"
