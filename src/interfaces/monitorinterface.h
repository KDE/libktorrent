/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTMONITORINTERFACE_H
#define BTMONITORINTERFACE_H

#include <ktorrent_export.h>

namespace bt
{
class ChunkDownloadInterface;
class PeerInterface;
class TorrentFileInterface;

/*!
 * \author Joris Guisson
 * \brief Interface for classes who want to monitor a TorrentInterface.
 *
 * Classes who want to keep track of all peers currently connected for a given
 * torrent and all chunks who are currently downloading can implement this interface.
 */
class KTORRENT_EXPORT MonitorInterface
{
public:
    MonitorInterface();
    virtual ~MonitorInterface();

    /*!
     * A peer has been added.
     * \param peer The peer
     */
    virtual void peerAdded(PeerInterface *peer) = 0;

    /*!
     * A peer has been removed.
     * \param peer The peer
     */
    virtual void peerRemoved(PeerInterface *peer) = 0;

    /*!
     * The download of a chunk has been started.
     * \param cd The ChunkDownload
     */
    virtual void downloadStarted(ChunkDownloadInterface *cd) = 0;

    /*!
     * The download of a chunk has been stopped.
     * \param cd The ChunkDownload
     */
    virtual void downloadRemoved(ChunkDownloadInterface *cd) = 0;

    /*!
     * The download has been stopped.
     */
    virtual void stopped() = 0;

    /*!
     * The download has been deleted.
     */
    virtual void destroyed() = 0;

    /*!
     * The download percentage of a file has changed.
     * \param file The file
     * \param percentage The percentage
     */
    virtual void filePercentageChanged(TorrentFileInterface *file, float percentage) = 0;

    /*!
     * Preview status of a file has changed.
     * \param file The file
     * \param preview Whether or not it is available
     */
    virtual void filePreviewChanged(TorrentFileInterface *file, bool preview) = 0;
};

}

#endif
