/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTUPLOADER_H
#define BTUPLOADER_H

#include "globals.h"
#include <peer/peermanager.h>

namespace bt
{
class Peer;
class ChunkManager;

/*!
 * \author Joris Guisson
 *
 * Class which manages the uploading of data. It has a PeerUploader for
 * each Peer.
 */
class Uploader : public QObject, public PeerManager::PeerVisitor
{
    Q_OBJECT
public:
    /*!
     * Constructor, sets the ChunkManager and PeerManager.
     * \param cman The ChunkManager
     * \param pman The PeerManager
     */
    Uploader(ChunkManager &cman, PeerManager &pman);
    ~Uploader() override;

    //! Get the number of bytes uploaded.
    Uint64 bytesUploaded() const
    {
        return uploaded;
    }

    //! Get the upload rate of all Peers combined.
    Uint32 uploadRate() const;

    //! Set the number of bytes which have been uploaded.
    void setBytesUploaded(Uint64 b)
    {
        uploaded = b;
    }

public Q_SLOTS:
    /*!
     * Update every PeerUploader.
     */
    void update();

private:
    void visit(const bt::Peer::Ptr p) override;

private:
    ChunkManager &cman;
    PeerManager &pman;
    Uint64 uploaded;
};

}

#endif
