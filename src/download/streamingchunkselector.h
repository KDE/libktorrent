/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_STREAMINGCHUNKSELECTOR_H
#define BT_STREAMINGCHUNKSELECTOR_H

#include <download/chunkselector.h>
#include <ktorrent_export.h>
#include <set>

namespace bt
{
/**
    ChunkSelector which supports streaming mode.
    It has a range of chunks which are to be downloaded sequentially. And it has a cursor, to support jumping around
    in the stream.
 */
class KTORRENT_EXPORT StreamingChunkSelector : public bt::ChunkSelector
{
public:
    StreamingChunkSelector();
    ~StreamingChunkSelector() override;

    void init(ChunkManager *cman, Downloader *downer, PeerManager *pman) override;
    bool select(bt::PieceDownloader *pd, bt::Uint32 &chunk) override;
    void dataChecked(const bt::BitSet &ok_chunks, Uint32 from, Uint32 to) override;
    void reincluded(bt::Uint32 from, bt::Uint32 to) override;
    void reinsert(bt::Uint32 chunk) override;
    bool selectRange(bt::Uint32 &from, bt::Uint32 &to, bt::Uint32 max_len) override;

    /// Get the critical window size in chunks
    Uint32 criticialWindowSize() const
    {
        return critical_window_size;
    }

    /**
        Set the range to be downloaded sequentially.
        The cursor will be initialized to the first of the range.
        @param from Start of range
        @param to End of range
     */
    void setSequentialRange(bt::Uint32 from, bt::Uint32 to);

    /// Set the cursor location
    void setCursor(bt::Uint32 chunk);

private:
    void updateRange();
    void initRange();
    bool selectFromPreview(bt::PieceDownloader *pd, bt::Uint32 &chunk);

private:
    bt::Uint32 range_start;
    bt::Uint32 range_end;
    bt::Uint32 cursor;
    bt::Uint32 critical_window_size;
    std::list<Uint32> range;
    std::set<Uint32> preview_chunks;
};

}

#endif // BT_STREAMINGCHUNKSELECTOR_H
