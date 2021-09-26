/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_MAGNETDOWNLOADER_H
#define BT_MAGNETDOWNLOADER_H

#include "magnetlink.h"
#include <QObject>
#include <kio/job.h>
#include <ktorrent_export.h>
#include <torrent/torrent.h>
#include <tracker/tracker.h>

namespace dht
{
class DHTPeerSource;
}

namespace bt
{
class Peer;
class PeerManager;

/**
    Class which tries to download the metadata associated to a MagnetLink
    It basically has a Tracker (optional), a DHTPeerSource and a PeerManager.
    With these it tries to find peers, connect to them and download the metadata.
*/
class KTORRENT_EXPORT MagnetDownloader : public QObject, public TrackerDataSource
{
    Q_OBJECT
public:
    MagnetDownloader(const MagnetLink &mlink, QObject *parent);
    ~MagnetDownloader() override;

    /**
        Update the MagnetDownloader
    */
    void update();

    /// Is the magnet download running
    bool running() const;

    /// How many peers are we connected to
    Uint32 numPeers() const;

    /// Get the MagnetLink
    const MagnetLink &magnetLink() const
    {
        return mlink;
    }

    /**
    Start the MagnetDownloader, this will enable DHT.
    */
    void start();

    /**
    Stop the MagnetDownloader
    */
    void stop();

Q_SIGNALS:
    /**
        Emitted when downloading the metadata was succesfull.
    */
    void foundMetadata(bt::MagnetDownloader *self, const QByteArray &metadata);

private:
    void onNewPeer(Peer *p);
    void onMetadataDownloaded(const QByteArray &data);
    void onTorrentDownloaded(KJob *);
    void dhtStarted();
    void dhtStopped();

    Uint64 bytesDownloaded() const override;
    Uint64 bytesUploaded() const override;
    Uint64 bytesLeft() const override;
    const SHA1Hash &infoHash() const override;
    bool isPartialSeed() const override;

    MagnetLink mlink;
    QList<Tracker *> trackers;
    PeerManager *pman;
    dht::DHTPeerSource *dht_ps;
    QByteArray metadata;
    Torrent tor;
    bool found;
};

}

#endif // BT_MAGNETDOWNLOADER_H
