/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "peeruploader.h"
#include "peer.h"
#include <diskio/chunkmanager.h>
#include <set>
#include <torrent/torrent.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/sha1hash.h>

namespace bt
{
PeerUploader::PeerUploader(Peer *peer)
    : peer(peer)
    , uploaded(0)
{
}

PeerUploader::~PeerUploader()
{
}

void PeerUploader::addRequest(const Request &r)
{
    requests.push_back(r);
}

void PeerUploader::removeRequest(const Request &r)
{
    requests.removeAll(r);
}

Uint32 PeerUploader::handleRequests(ChunkManager &cman)
{
    Uint32 ret = uploaded;
    uploaded = 0;

    // if we have choked the peer do not upload
    if (peer->areWeChoked()) {
        return ret;
    }

    while (requests.size() > 0) {
        Request r = requests.front();

        Chunk *c = cman.getChunk(r.getIndex());
        if (c && c->getStatus() == Chunk::ON_DISK) {
            if (!peer->sendChunk(r.getIndex(), r.getOffset(), r.getLength(), c)) {
                if (peer->getStats().fast_extensions) {
                    peer->sendReject(r);
                }
            }
        } else {
            // remove requests we can't satisfy
            Out(SYS_CON | LOG_DEBUG) << "Cannot satisfy request" << endl;
            if (peer->getStats().fast_extensions) {
                peer->sendReject(r);
            }
        }

        requests.pop_front();
    }

    return ret;
}

void PeerUploader::clearAllRequests()
{
    peer->clearPendingPieceUploads();
    if (peer->getStats().fast_extensions) {
        // reject all requests
        // if the peer supports fast extensions,
        // choke doesn't mean reject all
        for (const Request &r : std::as_const(requests)) {
            peer->sendReject(r);
        }
    }
    requests.clear();
}

Uint32 PeerUploader::getNumRequests() const
{
    return requests.size();
}
}
