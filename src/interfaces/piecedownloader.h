/*
    SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPIECEDOWNLOADER_H
#define BTPIECEDOWNLOADER_H

#include <QObject>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
class Piece;
class Request;

/*!
 * Interface for all things which want to download pieces from something.
 * \author Joris Guisson
 */
class KTORRENT_EXPORT PieceDownloader : public QObject
{
    Q_OBJECT
public:
    PieceDownloader();
    ~PieceDownloader() override;

    /*!
     * Grab the Peer, indicates how many ChunkDownload's
     * are using this PeerDownloader.
     * \return The number of times this PeerDownloader was grabbed
     */
    int grab();

    /*!
     * When a ChunkDownload is ready with this PeerDownloader,
     * it will release it, so that others can use it.
     */
    void release();

    //! Get the number of times this PeerDownloader was grabbed.
    int getNumGrabbed() const
    {
        return grabbed;
    }

    /*!
     * Send a Request.
     * \param req The Request
     */
    virtual void download(const bt::Request &req) = 0;

    /*!
     * Cancel a Request.
     * \param req The Request
     */
    virtual void cancel(const bt::Request &req) = 0;

    /*!
     * Cancel all Requests
     */
    virtual void cancelAll() = 0;

    /*!
     * Get the name of the PieceDownloader
     * This is something which can be shown in the GUI.
     * For a regular PeerDownloader, this should be the client and version.
     * For a webseed this should be the URL
     */
    virtual QString getName() const = 0;

    /*!
     * Get the current download rate.
     * \return The download rate in bytes/sec
     */
    virtual bt::Uint32 getDownloadRate() const = 0;

    /*!
     * See if the PieceDownloader is choked, can be overwritten by subclasses.
     * \return Whether or not the PieceDownloader is choked
     */
    virtual bool isChoked() const
    {
        return false;
    }

    /*!
     * Whether or not we can add another request.
     */
    virtual bool canAddRequest() const = 0;

    /*!
     * Whether or not we can download another chunk from this.
     */
    virtual bool canDownloadChunk() const = 0;

    //! See if this PieceDownloader has nearly finished a chunk
    bool isNearlyDone() const
    {
        return getNumGrabbed() == 1 && nearly_done;
    }

    //! Set the nearly done status of the PeerDownloader
    void setNearlyDone(bool nd)
    {
        nearly_done = nd;
    }

    /*!
     * See if the PieceDownloader has a Chunk.
     * By default this returns true, but it can be
     * overridden by subclasses.
     * \param idx The Chunk's index
     */
    virtual bool hasChunk(bt::Uint32 idx) const
    {
        Q_UNUSED(idx);
        return true;
    }

    /*!
     * Check if requests have timedout
     */
    virtual void checkTimeouts() = 0;

Q_SIGNALS:
    /*!
     * Emitted when a request takes longer then 60 seconds to download.
     * The sender of the request will have to request it again. This does not apply for
     * unsent requests. Their timestamps will be updated when they get transmitted.
     * \param r The request
     */
    void timedout(const bt::Request &r);

    /*!
     * A request was rejected.
     * \param req The Request
     */
    void rejected(const bt::Request &req);

private:
    int grabbed;
    bool nearly_done;
};

}

#endif
