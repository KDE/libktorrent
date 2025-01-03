/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "trackermanager.h"
#include <KLocalizedString>
#include <QFile>
#include <QTextStream>
#include <peer/peermanager.h>
#include <torrent/torrent.h>
#include <torrent/torrentcontrol.h>
#include <tracker/httptracker.h>
#include <tracker/tracker.h>
#include <tracker/udptracker.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
TrackerManager::TrackerManager(bt::TorrentControl *tor, PeerManager *pman)
    : tor(tor)
    , pman(pman)
    , curr(nullptr)
    , started(false)
{
    trackers.setAutoDelete(true);
    no_save_custom_trackers = false;

    const TrackerTier *t = tor->getTorrent().getTrackerList();
    int tier = 1;
    while (t) {
        // add all standard trackers
        const QList<QUrl> &tr = t->urls;
        QList<QUrl>::const_iterator i = tr.begin();
        while (i != tr.end()) {
            addTracker(*i, false, tier);
            ++i;
        }

        tier++;
        t = t->next;
    }

    // load custom trackers
    loadCustomURLs();
    // Load status of each tracker
    loadTrackerStatus();

    if (tor->getStats().priv_torrent)
        switchTracker(selectTracker());
}

TrackerManager::~TrackerManager()
{
    saveCustomURLs();
    saveTrackerStatus();
}

TrackerInterface *TrackerManager::getCurrentTracker() const
{
    return curr;
}

bool TrackerManager::noTrackersReachable() const
{
    if (tor->getStats().priv_torrent) {
        return curr ? curr->trackerStatus() == TRACKER_ERROR : false;
    } else {
        int enabled = 0;

        // If all trackers have an ERROR status, and there is at least one
        // enabled, we must return true;
        for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
            if (i->second->isEnabled()) {
                if (i->second->trackerStatus() != TRACKER_ERROR)
                    return false;
                enabled++;
            }
        }

        return enabled > 0;
    }
}

void addTrackerStatusToInfo(const Tracker *tr, TrackersStatusInfo &info)
{
    if (tr) {
        info.trackers_count++;

        if (tr->trackerStatus() == TRACKER_ERROR) {
            info.errors++;
            if (tr->timeOut())
                info.timeout_errors++;
        }
        if (tr->hasWarning())
            info.warnings++;
    }
}

TrackersStatusInfo TrackerManager::getTrackersStatusInfo() const
{
    TrackersStatusInfo tsi;
    tsi.trackers_count = tsi.errors = tsi.timeout_errors = tsi.warnings = 0;

    if (tor->getStats().priv_torrent) {
        addTrackerStatusToInfo(curr, tsi);
        return tsi;
    }

    for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i)
        if (i->second->isEnabled())
            addTrackerStatusToInfo(i->second, tsi);
    return tsi;
}

void TrackerManager::setCurrentTracker(bt::TrackerInterface *t)
{
    if (!tor->getStats().priv_torrent)
        return;

    Tracker *trk = (Tracker *)t;
    if (!trk)
        return;

    if (curr != trk) {
        if (curr)
            curr->stop();
        switchTracker(trk);
        trk->start();
    }
}

void TrackerManager::setCurrentTracker(const QUrl &url)
{
    Tracker *trk = trackers.find(url);
    if (trk)
        setCurrentTracker(trk);
}

QList<TrackerInterface *> TrackerManager::getTrackers()
{
    QList<TrackerInterface *> ret;
    for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
        ret.append(i->second);
    }

    return ret;
}

TrackerInterface *TrackerManager::addTracker(const QUrl &url, bool custom, int tier)
{
    if (trackers.contains(url))
        return nullptr;

    Tracker *trk = nullptr;
    if (url.scheme() == QLatin1String("udp"))
        trk = new UDPTracker(url, this, tor->getTorrent().getPeerID(), tier);
    else if (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"))
        trk = new HTTPTracker(url, this, tor->getTorrent().getPeerID(), tier);
    else
        return nullptr;

    addTracker(trk);
    if (custom) {
        custom_trackers.append(url);
        if (!no_save_custom_trackers) {
            saveCustomURLs();
            saveTrackerStatus();
        }
    }

    return trk;
}

bool TrackerManager::removeTracker(bt::TrackerInterface *t)
{
    return removeTracker(t->trackerURL());
}

bool TrackerManager::removeTracker(const QUrl &url)
{
    if (!custom_trackers.contains(url))
        return false;

    custom_trackers.removeAll(url);
    Tracker *trk = trackers.find(url);
    if (trk && curr == trk && tor->getStats().priv_torrent) {
        // do a timed delete on the tracker, so the stop signal
        // has plenty of time to reach it
        trk->stop();
        trk->timedDelete(10 * 1000);
        trackers.setAutoDelete(false);
        trackers.erase(url);
        trackers.setAutoDelete(true);

        if (trackers.count() > 0) {
            switchTracker(selectTracker());
            if (curr)
                curr->start();
        }
    } else {
        // just delete if not the current one
        trackers.erase(url);
    }
    saveCustomURLs();
    return true;
}

bool TrackerManager::canRemoveTracker(bt::TrackerInterface *t)
{
    return custom_trackers.contains(t->trackerURL());
}

void TrackerManager::restoreDefault()
{
    QList<QUrl>::iterator i = custom_trackers.begin();
    while (i != custom_trackers.end()) {
        Tracker *t = trackers.find(*i);
        if (t) {
            if (t->isStarted())
                t->stop();

            if (curr == t && tor->getStats().priv_torrent) {
                curr = nullptr;
                trackers.erase(*i);
            } else {
                trackers.erase(*i);
            }
        }
        ++i;
    }

    custom_trackers.clear();
    saveCustomURLs();
    if (tor->getStats().priv_torrent && curr == nullptr)
        switchTracker(selectTracker());
}

void TrackerManager::addTracker(Tracker *trk)
{
    trackers.insert(trk->trackerURL(), trk);
    connect(trk, &Tracker::peersReady, pman, &PeerManager::peerSourceReady);
    connect(trk, &Tracker::scrapeDone, tor, &TorrentControl::trackerScrapeDone);
    connect(trk, &Tracker::requestOK, this, &TrackerManager::onTrackerOK);
    connect(trk, &Tracker::requestFailed, this, &TrackerManager::onTrackerError);
}

void TrackerManager::start()
{
    if (started)
        return;

    if (tor->getStats().priv_torrent) {
        if (!curr) {
            if (trackers.count() > 0) {
                switchTracker(selectTracker());
                if (curr)
                    curr->start();
            }
        } else {
            curr->start();
        }
    } else {
        for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
            if (i->second->isEnabled())
                i->second->start();
        }
    }

    started = true;
}

void TrackerManager::stop(bt::WaitJob *wjob)
{
    if (!started)
        return;

    started = false;
    if (tor->getStats().priv_torrent) {
        if (curr)
            curr->stop(wjob);

        for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
            i->second->reset();
        }
    } else {
        for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
            i->second->stop(wjob);
            i->second->reset();
        }
    }
}

void TrackerManager::completed()
{
    if (tor->getStats().priv_torrent) {
        if (curr)
            curr->completed();
    } else {
        for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
            i->second->completed();
        }
    }
}

void TrackerManager::scrape()
{
    for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
        i->second->scrape();
    }
}

void TrackerManager::manualUpdate()
{
    if (tor->getStats().priv_torrent) {
        if (curr) {
            curr->manualUpdate();
        }
    } else {
        for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
            if (i->second->isEnabled())
                i->second->manualUpdate();
        }
    }
}

void TrackerManager::saveCustomURLs()
{
    QString trackers_file = tor->getTorDir() + QLatin1String("trackers");
    QFile file(trackers_file);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QTextStream stream(&file);
    for (const QUrl &url : std::as_const(custom_trackers))
        stream << url.toDisplayString() << Qt::endl;
}

void TrackerManager::loadCustomURLs()
{
    QString trackers_file = tor->getTorDir() + QLatin1String("trackers");
    QFile file(trackers_file);
    if (!file.open(QIODevice::ReadOnly))
        return;

    no_save_custom_trackers = true;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        addTracker(QUrl(stream.readLine()), true);
    }
    no_save_custom_trackers = false;
}

void TrackerManager::saveTrackerStatus()
{
    QString status_file = tor->getTorDir() + QLatin1String("tracker_status");
    QFile file(status_file);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QTextStream stream(&file);
    PtrMap<QUrl, Tracker>::iterator i = trackers.begin();
    while (i != trackers.end()) {
        QUrl url = i->first;
        Tracker *trk = i->second;

        stream << (trk->isEnabled() ? "1:" : "0:") << url.toDisplayString() << Qt::endl;
        ++i;
    }
}

void TrackerManager::loadTrackerStatus()
{
    QString status_file = tor->getTorDir() + QLatin1String("tracker_status");
    QFile file(status_file);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.size() < 2)
            continue;

        if (line[0] == '0'_L1) {
            Tracker *trk = trackers.find(QUrl(line.mid(2))); // url starts at the second char
            if (trk)
                trk->setEnabled(false);
        }
    }
}

Tracker *TrackerManager::selectTracker()
{
    Tracker *n = nullptr;
    PtrMap<QUrl, Tracker>::iterator i = trackers.begin();
    while (i != trackers.end()) {
        Tracker *t = i->second;
        if (t->isEnabled()) {
            if (!n)
                n = t;
            else if (t->failureCount() < n->failureCount())
                n = t;
            else if (t->failureCount() == n->failureCount())
                n = t->getTier() < n->getTier() ? t : n;
        }
        ++i;
    }

    if (n) {
        Out(SYS_TRK | LOG_DEBUG) << "Selected tracker " << n->trackerURL().toString() << " (tier = " << n->getTier() << ")" << endl;
    }

    return n;
}

void TrackerManager::onTrackerError(const QString &err)
{
    Q_UNUSED(err);
    if (!started)
        return;

    if (!tor->getStats().priv_torrent) {
        Tracker *trk = (Tracker *)sender();
        trk->handleFailure();
    } else {
        Tracker *trk = (Tracker *)sender();
        if (trk == curr) {
            // select an other tracker
            trk = selectTracker();
            if (trk == curr) { // if we can't find another handle the failure
                trk->handleFailure();
            } else {
                curr->stop();
                switchTracker(trk);
                if (curr->failureCount() > 0)
                    curr->handleFailure();
                else
                    curr->start();
            }
        } else
            trk->handleFailure();
    }
}

void TrackerManager::onTrackerOK()
{
    Tracker *tracker = (Tracker *)sender();
    if (tracker->isStarted())
        tracker->scrape();
}

void TrackerManager::updateCurrentManually()
{
    if (!curr)
        return;

    curr->manualUpdate();
}

void TrackerManager::switchTracker(Tracker *trk)
{
    if (curr == trk)
        return;

    curr = trk;
    if (curr)
        Out(SYS_TRK | LOG_NOTICE) << "Switching to tracker " << trk->trackerURL() << endl;
}

Uint32 TrackerManager::getNumSeeders() const
{
    if (tor->getStats().priv_torrent) {
        return curr && curr->getNumSeeders() > 0 ? curr->getNumSeeders() : 0;
    }

    int r = 0;
    for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
        int v = i->second->getNumSeeders();
        if (v > r)
            r = v;
    }

    return r;
}

Uint32 TrackerManager::getNumLeechers() const
{
    if (tor->getStats().priv_torrent)
        return curr && curr->getNumLeechers() > 0 ? curr->getNumLeechers() : 0;

    int r = 0;
    for (PtrMap<QUrl, Tracker>::const_iterator i = trackers.begin(); i != trackers.end(); ++i) {
        int v = i->second->getNumLeechers();
        if (v > r)
            r = v;
    }

    return r;
}

void TrackerManager::setTrackerEnabled(const QUrl &url, bool enabled)
{
    Tracker *trk = trackers.find(url);
    if (!trk)
        return;

    trk->setEnabled(enabled);
    if (!enabled) {
        trk->stop();
        if (curr == trk) { // if the current tracker is disabled, switch to another one
            switchTracker(selectTracker());
            if (curr)
                curr->start();
        }
    } else {
        // start tracker if necessary
        if (!tor->getStats().priv_torrent && started)
            trk->start();
    }

    saveTrackerStatus();
}

Uint64 TrackerManager::bytesDownloaded() const
{
    const TorrentStats &s = tor->getStats();
    if (s.imported_bytes > s.bytes_downloaded)
        return 0;
    else
        return s.bytes_downloaded - s.imported_bytes;
}

Uint64 TrackerManager::bytesUploaded() const
{
    return tor->getStats().bytes_uploaded;
}

Uint64 TrackerManager::bytesLeft() const
{
    return tor->getStats().bytes_left;
}

const bt::SHA1Hash &TrackerManager::infoHash() const
{
    return tor->getInfoHash();
}

bool TrackerManager::isPartialSeed() const
{
    return pman->isPartialSeed();
}

}

#include "moc_trackermanager.cpp"
