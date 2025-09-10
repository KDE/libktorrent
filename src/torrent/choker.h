/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCHOKER_H
#define BTCHOKER_H

#include <QList>
#include <ktorrent_export.h>
#include <peer/peer.h>
#include <util/constants.h>

namespace bt
{
const Uint32 UNDEFINED_ID = 0xFFFFFFFF;

class PeerManager;
class ChunkManager;
struct TorrentStats;

/*!
 * \headerfile torrent/chokealgorithm.h
 * \brief Base class for all choke algorithms.
 */
class ChokeAlgorithm
{
protected:
    Uint32 opt_unchoked_peer_id;

public:
    ChokeAlgorithm();
    virtual ~ChokeAlgorithm();

    /*!
     * Do the actual choking when we are still downloading.
     * \param pman The PeerManager
     * \param cman The ChunkManager
     * \param stats The torrent stats
     */
    virtual void doChokingLeechingState(PeerManager &pman, ChunkManager &cman, const TorrentStats &stats) = 0;

    /*!
     * Do the actual choking when we are seeding
     * \param pman The PeerManager
     * \param cman The ChunkManager
     * \param stats The torrent stats
     */
    virtual void doChokingSeedingState(PeerManager &pman, ChunkManager &cman, const TorrentStats &stats) = 0;

    //! Get the optimisticly unchoked peer ID
    [[nodiscard]] Uint32 getOptimisticlyUnchokedPeerID() const
    {
        return opt_unchoked_peer_id;
    }
};

/*!
 * \headerfile torrent/chokealgorithm.h
 * \author Joris Guisson
 * \brief Runs the ChokeAlgorithm for a given torrent.
 *
 * This class handles the choking and unchoking of Peer's.
 * This class needs to be updated every 10 seconds.
 */
class KTORRENT_EXPORT Choker
{
    ChokeAlgorithm *choke;
    PeerManager &pman;
    ChunkManager &cman;
    static Uint32 num_upload_slots;

public:
    Choker(PeerManager &pman, ChunkManager &cman);
    virtual ~Choker();

    /*!
     * Update which peers are choked or not.
     * \param have_all Indicates whether we have the entire file
     * \param stats Statistic of the torrent
     */
    void update(bool have_all, const TorrentStats &stats);

    //! Get the PeerID of the optimisticly unchoked peer.
    [[nodiscard]] Uint32 getOptimisticlyUnchokedPeerID() const
    {
        return choke->getOptimisticlyUnchokedPeerID();
    }

    //! Set the number of upload slots
    static void setNumUploadSlots(Uint32 n)
    {
        num_upload_slots = n;
    }

    //! Get the number of upload slots
    static Uint32 getNumUploadSlots()
    {
        return num_upload_slots;
    }
};

}

#endif
