/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "choker.h"
#include "advancedchokealgorithm.h"
#include <peer/peermanager.h>
#include <util/functions.h>

namespace bt
{
ChokeAlgorithm::ChokeAlgorithm()
    : opt_unchoked_peer_id(0)
{
}

ChokeAlgorithm::~ChokeAlgorithm()
{
}

/////////////////////////////////

Uint32 Choker::num_upload_slots = 2;

Choker::Choker(PeerManager &pman, ChunkManager &cman)
    : pman(pman)
    , cman(cman)
{
    choke = new AdvancedChokeAlgorithm();
}

Choker::~Choker()
{
    delete choke;
}

void Choker::update(bool have_all, const TorrentStats &stats)
{
    if (have_all)
        choke->doChokingSeedingState(pman, cman, stats);
    else
        choke->doChokingLeechingState(pman, cman, stats);
}

}
