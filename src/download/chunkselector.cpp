/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include "chunkselector.h"
#include "chunkdownload.h"
#include "downloader.h"
#include <algorithm>
#include <random>
#include <diskio/chunkmanager.h>
#include <interfaces/piecedownloader.h>
#include <peer/chunkcounter.h>
#include <peer/peer.h>
#include <peer/peermanager.h>
#include <stdlib.h>
#include <torrent/torrent.h>
#include <util/bitset.h>
#include <util/log.h>
#include <vector>

namespace bt
{
struct RareCmp {
    ChunkManager *cman;
    ChunkCounter &cc;
    bool warmup;

    RareCmp(ChunkManager *cman, ChunkCounter &cc, bool warmup)
        : cman(cman)
        , cc(cc)
        , warmup(warmup)
    {
    }

    bool operator()(Uint32 a, Uint32 b)
    {
        if (a >= cman->getNumChunks() || b >= cman->getNumChunks())
            return false;
        // the sorting is done on two criteria, priority and rareness
        Priority pa = cman->getChunk(a)->getPriority();
        Priority pb = cman->getChunk(b)->getPriority();
        if (pa == pb)
            return normalCmp(a, b); // if both have same priority compare on rareness
        else if (pa > pb) // pa has priority over pb, so select pa
            return true;
        else // pb has priority over pa, so select pb
            return false;
    }

    bool normalCmp(Uint32 a, Uint32 b)
    {
        // during warmup mode choose most common chunks
        if (!warmup)
            return cc.get(a) < cc.get(b);
        else
            return cc.get(a) > cc.get(b);
    }
};

ChunkSelector::ChunkSelector()
{
}

ChunkSelector::~ChunkSelector()
{
}

void ChunkSelector::init(ChunkManager *cman, Downloader *downer, PeerManager *pman)
{
    bt::ChunkSelectorInterface::init(cman, downer, pman);
    std::vector<Uint32> tmp;
    std::random_device rd;
    std::mt19937 g(rd());

    for (Uint32 i = 0; i < cman->getNumChunks(); i++) {
        if (!cman->getBitSet().get(i)) {
            tmp.push_back(i);
        }
    }
    std::shuffle(tmp.begin(),tmp.end(), g);
    // std::list does not support random_shuffle so we use a vector as a temporary storage
    // for the random_shuffle
    chunks.insert(chunks.begin(), tmp.begin(), tmp.end());
    sort_timer.update();
}

Uint32 ChunkSelector::leastPeers(const std::list<Uint32> &lp, Uint32 alternative, Uint32 max_peers_per_chunk)
{
    bool endgame = downer->endgameMode();

    Uint32 sel = lp.front();
    Uint32 cnt = downer->numDownloadersForChunk(sel);
    for (Uint32 i : lp) {
        Uint32 cnt_i = downer->numDownloadersForChunk(i);
        if (cnt_i < cnt) {
            sel = i;
            cnt = cnt_i;
        }
    }

    if (!endgame && downer->numDownloadersForChunk(sel) >= max_peers_per_chunk) {
        ChunkDownload *cd = downer->download(sel);
        if (!cd)
            return alternative;

        // if download speed is very small, use sel
        // even though the max_peers_per_chunk has been reached
        if (cd->getDownloadSpeed() < 100)
            return sel;
        else
            return alternative;
    }

    return sel;
}

bool ChunkSelector::select(PieceDownloader *pd, Uint32 &chunk)
{
    const BitSet &bs = cman->getBitSet();

    Uint32 sel = ~Uint32();
    Uint32 sel_dl = ~Uint32();
    Priority sel_prio = EXCLUDED;

    // sort the chunks every 2 seconds
    if (sort_timer.getElapsedSinceUpdate() > 2000) {
        bool warmup = cman->getNumChunks() - cman->chunksLeft() <= 4;
        chunks.sort(RareCmp(cman, pman->getChunkCounter(), warmup));
        sort_timer.update();
    }

    std::list<Uint32>::iterator itr = chunks.begin();
    while (itr != chunks.end()) {
        Uint32 i = *itr;
        Chunk *c = cman->getChunk(*itr);

        // if we have the chunk remove it from the list
        if (c->isExcludedForDownloading() || c->isExcluded() || bs.get(i)) {
            std::list<Uint32>::iterator tmp = itr;
            ++itr;
            chunks.erase(tmp);
        } else if (c->getPriority() < sel_prio) {
            // we've already found a suitable chunk at a higher priority, so select that one
            break;
        } else if (pd->hasChunk(i)) {
            // pd has to have the selected chunk and it needs to be not excluded
            Uint32 dl = downer->numDownloadersForChunk(i);
            if (dl == 0) {
                // we found a chunk that has no downloaders, so select it
                sel = i;
                break;
            }
            // we found a chunk that has downloaders; remember it if it has fewer
            // downloaders than the chunk we're already remembering (if any) and:
            //   - it has fewer than max_peers_per_chunk downloaders, or
            //   - we're in endgame mode, or
            //   - it is downloading very slowly
            if (dl < sel_dl) {
                const Uint32 max_peers_per_chunk = c->isPreview() ? 3 : 2;
                ChunkDownload *cd;
                if (dl < max_peers_per_chunk || downer->endgameMode() ||
                        ((cd = downer->download(i)) && cd->getDownloadSpeed() < 100))
                {
                    sel = i;
                    sel_dl = dl;
                    sel_prio = c->getPriority();
                }
            }
            ++itr;
        } else
            ++itr;
    }

    if (sel >= cman->getNumChunks())
        return false;

    chunk = sel;
    return true;
}

void ChunkSelector::dataChecked(const BitSet &ok_chunks, Uint32 from, Uint32 to)
{
    for (Uint32 i = from; i < ok_chunks.getNumBits() && i <= to; i++) {
        bool in_chunks = std::find(chunks.begin(), chunks.end(), i) != chunks.end();
        if (in_chunks && ok_chunks.get(i)) {
            // if we have the chunk, remove it from the chunks list
            chunks.remove(i);
        } else if (!in_chunks && !ok_chunks.get(i)) {
            // if we don't have the chunk, add it to the list if it wasn't allrready in there
            chunks.push_back(i);
        }
    }
}

void ChunkSelector::reincluded(Uint32 from, Uint32 to)
{
    // lets do a safety check first
    if (from >= cman->getNumChunks() || to >= cman->getNumChunks()) {
        Out(SYS_DIO | LOG_NOTICE) << "Internal error in chunkselector" << endl;
        return;
    }

    for (Uint32 i = from; i <= to; i++) {
        bool in_chunks = std::find(chunks.begin(), chunks.end(), i) != chunks.end();
        if (!in_chunks && cman->getChunk(i)->getStatus() != Chunk::ON_DISK) {
            //  Out(SYS_DIO|LOG_DEBUG) << "ChunkSelector::reIncluded " << i << endl;
            chunks.push_back(i);
        }
    }
}

void ChunkSelector::reinsert(Uint32 chunk)
{
    bool in_chunks = std::find(chunks.begin(), chunks.end(), chunk) != chunks.end();
    if (!in_chunks)
        chunks.push_back(chunk);
}

struct ChunkRange {
    Uint32 start;
    Uint32 len;
};

bool ChunkSelector::selectRange(Uint32 &from, Uint32 &to, Uint32 max_len)
{
    Uint32 max_range_len = max_len;

    const BitSet &bs = cman->getBitSet();
    Uint32 num_chunks = cman->getNumChunks();
    ChunkRange curr = {0, 0};
    ChunkRange best = {0, 0};

    for (Uint32 i = 0; i < num_chunks; i++) {
        if (!bs.get(i) && downer->canDownloadFromWebSeed(i)) {
            if (curr.start == 0 && curr.len == 0) { // we have a new current range
                curr.start = i;
                curr.len = 1;
            } else
                curr.len++; // expand the current range

            if (curr.len >= max_range_len) {
                // current range is bigger then the maximum range length, select it
                from = curr.start;
                to = curr.start + curr.len - 1;
                return true;
            } else if (i == num_chunks - 1) {
                if (curr.start > 0 && curr.len > best.len)
                    best = curr;
            }
        } else {
            // see if the current range has ended
            // and if it is better then the best range
            if (curr.start > 0 && curr.len > best.len)
                best = curr; // we have a new best range

            curr.start = curr.len = 0; // reset the current range
        }
    }

    // end of the list, so select the best
    if (best.len) {
        from = best.start;
        to = best.start + best.len - 1;
        return true;
    }
    return false;
}
}
