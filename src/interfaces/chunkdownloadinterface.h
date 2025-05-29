/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCHUNKDOWNLOADINTERFACE_H
#define BTCHUNKDOWNLOADINTERFACE_H

#include <QString>
#include <util/constants.h>

namespace bt
{
/*!
 * \author Joris Guisson
 * \brief Interface for a ChunkDownload
 *
 * This class provides the interface for a ChunkDownload object.
 */
class ChunkDownloadInterface
{
public:
    ChunkDownloadInterface();
    virtual ~ChunkDownloadInterface();

    struct Stats {
        //! The PeerID of the current downloader
        QString current_peer_id;
        //! The current download speed
        bt::Uint32 download_speed;
        //! The index of the chunk
        bt::Uint32 chunk_index;
        //! The number of pieces of the chunk which have been downloaded
        bt::Uint32 pieces_downloaded;
        //! The total number of pieces of the chunk
        bt::Uint32 total_pieces;
        //! The number of assigned downloaders
        bt::Uint32 num_downloaders;
    };

    virtual void getStats(Stats &s) = 0;
};

}

#endif
