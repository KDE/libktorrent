/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "peerinterface.h"
#include <util/functions.h>

namespace bt
{
PeerInterface::PeerInterface(const PeerID &peer_id, Uint32 num_chunks)
    : peer_id(peer_id)
    , pieces(num_chunks)
{
    stats.interested = false;
    stats.am_interested = false;
    stats.choked = true;
    stats.interested = false;
    stats.am_interested = false;
    stats.download_rate = 0;
    stats.upload_rate = 0;
    stats.perc_of_file = 0;
    stats.snubbed = false;
    stats.dht_support = false;
    stats.fast_extensions = false;
    stats.extension_protocol = false;
    stats.bytes_downloaded = stats.bytes_uploaded = 0;
    stats.aca_score = 0.0;
    stats.has_upload_slot = false;
    stats.num_up_requests = stats.num_down_requests = 0;
    stats.encrypted = false;
    stats.local = false;
    stats.max_request_queue = 0;
    stats.time_choked = CurrentTime();
    stats.time_unchoked = 0;
    stats.partial_seed = false;
    killed = false;
    paused = false;
}

PeerInterface::~PeerInterface()
{
}

}
