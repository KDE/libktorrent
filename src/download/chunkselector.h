/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCHUNKSELECTOR_H
#define BTCHUNKSELECTOR_H

#include <interfaces/chunkselectorinterface.h>
#include <list>
#include <util/timer.h>

namespace bt
{
class BitSet;
class ChunkManager;
class Downloader;
class PeerManager;
class PieceDownloader;

/*!
 * \author Joris Guisson
 *
 * \brief Selects which Chunks to download.
 */
class ChunkSelector : public ChunkSelectorInterface
{
    std::list<Uint32> chunks;
    Timer sort_timer;

public:
    ChunkSelector();
    ~ChunkSelector() override;

    void init(ChunkManager *cman, Downloader *downer, PeerManager *pman) override;

    /*!
     * Select which chunk to download for a PieceDownloader.
     * \param pd The PieceDownloader
     * \param chunk Index of chunk gets stored here
     * \return true upon succes, false otherwise
     */
    bool select(PieceDownloader *pd, Uint32 &chunk) override;

    /*!
     * Data has been checked, and these chunks are OK.
     * \param ok_chunks The ok_chunks
     * \param from The first chunk checked
     * \param to The last chunk checked
     */
    void dataChecked(const bt::BitSet &ok_chunks, bt::Uint32 from, bt::Uint32 to) override;

    /*!
     * A range of chunks has been reincluded.
     * \param from The first chunk
     * \param to The last chunk
     */
    void reincluded(Uint32 from, Uint32 to) override;

    /*!
     * Reinsert a chunk.
     * \param chunk The chunk
     */
    void reinsert(Uint32 chunk) override;

    bool selectRange(Uint32 &from, Uint32 &to, Uint32 max_len) override;

protected:
    Uint32 leastPeers(const std::list<Uint32> &lp, Uint32 alternative, Uint32 max_peers_per_chunk);
};

}

#endif
