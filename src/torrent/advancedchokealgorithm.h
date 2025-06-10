/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTADVANCEDCHOKEALGORITHM_H
#define BTADVANCEDCHOKEALGORITHM_H

#include "choker.h"
#include <peer/peer.h>

namespace bt
{
struct TorrentStats;

/*!
    \author Joris Guisson <joris.guisson@gmail.com>
    \brief Choking algorithm that prioritizes local and new peers, and deprioritizes peers that haven't uploaded much data or have low bandwidth.
*/
class AdvancedChokeAlgorithm : public ChokeAlgorithm
{
public:
    AdvancedChokeAlgorithm();
    ~AdvancedChokeAlgorithm() override;

    void doChokingLeechingState(PeerManager &pman, ChunkManager &cman, const TorrentStats &stats) override;
    void doChokingSeedingState(PeerManager &pman, ChunkManager &cman, const TorrentStats &stats) override;

private:
    bool calcACAScore(Peer *p, ChunkManager &cman, const TorrentStats &stats);
    Peer *updateOptimisticPeer(PeerManager &pman, const QList<Peer *> &ppl);
    void doUnchoking(const QList<Peer *> &ppl, Peer *poup);

private:
    TimeStamp last_opt_sel_time; // last time we updated the optimistic unchoked peer
};

}

#endif
