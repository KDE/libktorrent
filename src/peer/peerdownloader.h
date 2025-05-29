/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEERDOWNLOADER_H
#define BTPEERDOWNLOADER_H

#include <QList>
#include <QObject>
#include <download/request.h>
#include <interfaces/piecedownloader.h>

namespace bt
{
class Peer;
class Request;
class Piece;

/*!
 * Request with a timestamp.
 */
struct TimeStampedRequest {
    Request req;
    TimeStamp time_stamp;

    TimeStampedRequest();

    /*!
     * Constructor, set the request and calculate the timestamp.
     * \param r The Request
     */
    TimeStampedRequest(const Request &r);

    /*!
     * Copy constructor, copy the request and the timestamp
     * \param t The TimeStampedRequest
     */
    TimeStampedRequest(const TimeStampedRequest &t);

    //! Destructor
    ~TimeStampedRequest();

    //! Smaller then operator, uses timestamps to compare
    bool operator<(const TimeStampedRequest &t) const
    {
        return time_stamp < t.time_stamp;
    }

    /*!
     * Equality operator, compares requests only.
     * \param r The Request
     * \return true if equal
     */
    bool operator==(const Request &r) const;

    /*!
     * Equality operator, compares requests only.
     * \param r The Request
     * \return true if equal
     */
    bool operator==(const TimeStampedRequest &r) const;

    /*!
     * Assignment operator.
     * \param r The Request to copy
     * \return *this
     */
    TimeStampedRequest &operator=(const Request &r);

    /*!
     * Assignment operator.
     * \param r The TimeStampedRequest to copy
     * \return *this
     */
    TimeStampedRequest &operator=(const TimeStampedRequest &r);
};

/*!
 * \author Joris Guisson
 * \brief Class which downloads pieces from a Peer
 *
 * This class downloads Piece's from a Peer.
 */
class PeerDownloader : public PieceDownloader
{
    Q_OBJECT
public:
    /*!
     * Constructor, set the Peer
     * \param peer The Peer
     * \param chunk_size Size of a chunk in bytes
     */
    PeerDownloader(Peer *peer, Uint32 chunk_size);
    ~PeerDownloader() override;

    //! See if we can add a request to the wait_queue
    bool canAddRequest() const override;
    bool canDownloadChunk() const override;

    //! Get the number of active requests
    Uint32 getNumRequests() const;

    //! Is the Peer choked.
    bool isChoked() const override;

    //! Is NULL (is the Peer set)
    bool isNull() const
    {
        return peer == nullptr;
    }

    /*!
     * See if the Peer has a Chunk
     * \param idx The Chunk's index
     */
    bool hasChunk(Uint32 idx) const override;

    //! Get the Peer
    const Peer *getPeer() const
    {
        return peer;
    }

    /*!
     * Check for timed out requests.
     */
    void checkTimeouts() override;

    //! Get the maximum number of chunk downloads
    Uint32 getMaxChunkDownloads() const;

    /*!
     * The peer has been choked, all pending requests are rejected.
     * (except for allowed fast ones)
     */
    void choked();

    QString getName() const override;
    Uint32 getDownloadRate() const override;

    /*!
     * Called when a piece has arrived.
     * \param p The Piece
     */
    void piece(const Piece &p);

public Q_SLOTS:
    /*!
     * Send a Request. Note that the DownloadCap
     * may not allow this. (In which case it will
     * be stored temporarely in the unsent_reqs list)
     * \param req The Request
     */
    void download(const Request &req) override;

    /*!
     * Cancel a Request.
     * \param req The Request
     */
    void cancel(const Request &req) override;

    /*!
     * Cancel all Requests
     */
    void cancelAll() override;

    /*!
     * Handles a rejected request.
     * \param req
     */
    void onRejected(const Request &req);

    /*!
     * Send requests and manage wait queue
     */
    void update();

private Q_SLOTS:
    void peerDestroyed();

private:
    Peer *peer;
    QList<TimeStampedRequest> reqs;
    QList<Request> wait_queue;
    Uint32 max_wait_queue_size;
    Uint32 chunk_size;
};

}

#endif
