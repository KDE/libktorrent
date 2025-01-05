/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "magnetdownloader.h"
#include <dht/dhtbase.h>
#include <dht/dhtpeersource.h>
#include <peer/peer.h>
#include <peer/peermanager.h>
#include <torrent/globals.h>
#include <tracker/httptracker.h>
#include <tracker/udptracker.h>

#include <KIO/StoredTransferJob>

#include "bcodec/bdecoder.h"
#include "bcodec/bnode.h"
#include "util/error.h"

namespace bt
{
MagnetDownloader::MagnetDownloader(const bt::MagnetLink &mlink, QObject *parent)
    : QObject(parent)
    , mlink(mlink)
    , pman(nullptr)
    , dht_ps(nullptr)
    , tor(mlink.infoHash())
    , found(false)
{
    dht::DHTBase &dht_table = Globals::instance().getDHT();
    connect(&dht_table, &dht::DHTBase::started, this, &MagnetDownloader::dhtStarted);
    connect(&dht_table, &dht::DHTBase::stopped, this, &MagnetDownloader::dhtStopped);
}

MagnetDownloader::~MagnetDownloader()
{
    if (running())
        stop();
}

void MagnetDownloader::start()
{
    if (running())
        return;

    if (!mlink.torrent().isEmpty()) {
        KIO::StoredTransferJob *job = KIO::storedGet(QUrl(mlink.torrent()), KIO::NoReload, KIO::HideProgressInfo);
        connect(job, &KIO::StoredTransferJob::result, this, &MagnetDownloader::onTorrentDownloaded);
    }

    pman = new PeerManager(tor);
    connect(pman, &PeerManager::newPeer, this, &MagnetDownloader::onNewPeer);

    const QList<QUrl> trackers_list = mlink.trackers();
    for (const QUrl &url : trackers_list) {
        Tracker *tracker;
        if (url.scheme() == QLatin1String("udp"))
            tracker = new UDPTracker(url, this, tor.getPeerID(), 0);
        else
            tracker = new HTTPTracker(url, this, tor.getPeerID(), 0);
        trackers << tracker;
        connect(tracker, &Tracker::peersReady, pman, &PeerManager::peerSourceReady);
        tracker->start();
    }

    dht::DHTBase &dht_table = Globals::instance().getDHT();
    if (dht_table.isRunning()) {
        dht_ps = new dht::DHTPeerSource(dht_table, mlink.infoHash().truncated(), mlink.displayName());
        dht_ps->setRequestInterval(0); // Do not wait if the announce task finishes
        connect(dht_ps, &dht::DHTPeerSource::peersReady, pman, &PeerManager::peerSourceReady);
        dht_ps->start();
    }

    pman->start(false);
}

void MagnetDownloader::stop()
{
    if (!running())
        return;

    for (Tracker *tracker : std::as_const(trackers)) {
        tracker->stop();
        delete tracker;
    }
    trackers.clear();

    if (dht_ps) {
        dht_ps->stop();
        delete dht_ps;
        dht_ps = nullptr;
    }

    pman->stop();
    delete pman;
    pman = nullptr;
}

void MagnetDownloader::update()
{
    if (pman)
        pman->update();
}

bool MagnetDownloader::running() const
{
    return pman != nullptr;
}

Uint32 MagnetDownloader::numPeers() const
{
    return pman ? pman->getNumConnectedPeers() : 0;
}

void MagnetDownloader::onNewPeer(Peer *p)
{
    if (!p->getStats().extension_protocol) {
        // If the peer doesn't support the extension protocol,
        // kill it
        p->kill();
    } else {
        connect(p, &Peer::metadataDownloaded, this, &MagnetDownloader::onMetadataDownloaded);
    }
}

Uint64 MagnetDownloader::bytesDownloaded() const
{
    return 0;
}

Uint64 MagnetDownloader::bytesUploaded() const
{
    return 0;
}

Uint64 MagnetDownloader::bytesLeft() const
{
    return 0;
}

bool MagnetDownloader::isPartialSeed() const
{
    return false;
}

const bt::InfoHash &MagnetDownloader::infoHash() const
{
    return mlink.infoHash();
}

void MagnetDownloader::onTorrentDownloaded(KJob *job)
{
    if (!job)
        return;

    KIO::StoredTransferJob *stj = qobject_cast<KIO::StoredTransferJob *>(job);
    if (job->error()) {
        Out(SYS_GEN | LOG_DEBUG) << "Failed to download " << stj->url() << ": " << stj->errorString() << endl;
        return;
    }

    QByteArray data = stj->data();
    try {
        Torrent tor;
        tor.load(data, false);
        const TrackerTier *tier = tor.getTrackerList();
        while (tier) {
            mlink.tracker_urls.append(tier->urls);
            tier = tier->next.get();
        }
        onMetadataDownloaded(tor.getMetaData());
    } catch (...) {
        Out(SYS_GEN | LOG_NOTICE) << "Invalid torrent file from " << mlink.torrent() << endl;
    }
}

void MagnetDownloader::onMetadataDownloaded(const QByteArray &data)
{
    if (found)
        return;

    bt::SHA1Hash hash1 = bt::SHA1Hash::generate(data);
    bt::SHA2Hash hash2 = bt::SHA2Hash::generate(data);
    auto hash = bt::InfoHash(hash1, hash2);
    if (hash != mlink.infoHash()) {
        Out(SYS_GEN | LOG_NOTICE) << "Metadata downloaded, but hash check failed" << endl;
        return;
    }

    found = true;
    Out(SYS_GEN | LOG_IMPORTANT) << "Metadata downloaded" << endl;
    Q_EMIT foundMetadata(this, data);
    QTimer::singleShot(0, this, &MagnetDownloader::stop);
}

void MagnetDownloader::dhtStarted()
{
    if (running() && !dht_ps) {
        dht::DHTBase &dht_table = Globals::instance().getDHT();
        dht_ps = new dht::DHTPeerSource(dht_table, mlink.infoHash().truncated(), mlink.displayName());
        dht_ps->setRequestInterval(0); // Do not wait if the announce task finishes
        connect(dht_ps, &dht::DHTPeerSource::peersReady, pman, &PeerManager::peerSourceReady);
        dht_ps->start();
    }
}

void MagnetDownloader::dhtStopped()
{
    if (running() && dht_ps) {
        dht_ps->stop();
        delete dht_ps;
        dht_ps = nullptr;
    }
}

}

#include "moc_magnetdownloader.cpp"
