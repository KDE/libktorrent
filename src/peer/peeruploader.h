/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEERUPLOADER_H
#define BTPEERUPLOADER_H

#include <QList>
#include <download/request.h>

namespace bt
{
class Peer;
class ChunkManager;

const Uint32 ALLOWED_FAST_SIZE = 8;

/*!
 * \author Joris Guisson
 * \brief Uploads pieces to a Peer
 *
 * This class handles the uploading of pieces to a Peer. It keeps
 * track of a list of Request objects. All these Requests where sent
 * by the Peer. It will upload the pieces to the Peer, making sure
 * that the maximum upload rate isn't surpassed.
 */
class PeerUploader
{
    Peer *peer;
    QList<Request> requests;
    Uint32 uploaded;

public:
    /*!
     * Constructor. Set the Peer.
     * \param peer The Peer
     */
    PeerUploader(Peer *peer);
    virtual ~PeerUploader();

    /*!
     * Add a Request to the list of Requests.
     * \param r The Request
     */
    void addRequest(const Request &r);

    /*!
     * Remove a Request from the list of Requests.
     * \param r The Request
     */
    void removeRequest(const Request &r);

    /*!
     * Update the PeerUploader. This will check if there are Request, and
     * will try to handle them.
     * \param cman The ChunkManager
     * \return The number of bytes uploaded
     */
    Uint32 handleRequests(bt::ChunkManager &cman);

    //! Get the number of requests
    Uint32 getNumRequests() const;

    void addUploadedBytes(Uint32 bytes)
    {
        uploaded += bytes;
    }

    /*!
     * Clear all pending requests.
     */
    void clearAllRequests();
};

}

#endif
