/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "tracker.h"

#include <QRandomGenerator>
#include <QUrl>

#include "httptracker.h"
#include "udptracker.h"
#include <net/addressresolver.h>
#include <torrent/globals.h>
#include <torrent/server.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
static QString custom_ip;
static QString custom_ip_resolved;

const Uint32 INITIAL_WAIT_TIME = 30;
const Uint32 LONGER_WAIT_TIME = 300;
const Uint32 FINAL_WAIT_TIME = 1800;

Tracker::Tracker(const QUrl &url, TrackerDataSource *tds, const PeerID &id, int tier)
    : TrackerInterface(url)
    , tier(tier)
    , peer_id(id)
    , tds(tds)
{
    key = QRandomGenerator::global()->generate();
    connect(&reannounce_timer, &QTimer::timeout, this, &Tracker::manualUpdate);
    reannounce_timer.setSingleShot(true);
    bytes_downloaded_at_start = bytes_uploaded_at_start = 0;
}

Tracker::~Tracker()
{
}

void Tracker::setCustomIP(const QString &ip)
{
    if (custom_ip == ip)
        return;

    Out(SYS_TRK | LOG_NOTICE) << "Setting custom ip to " << ip << endl;
    custom_ip = ip;
    custom_ip_resolved = QString();
    if (ip.isNull())
        return;

    if (custom_ip.endsWith(QLatin1String(".i2p"))) {
        custom_ip_resolved = custom_ip;
    } else {
        net::Address addr;
        if (addr.setAddress(ip))
            custom_ip_resolved = custom_ip;
        else
            custom_ip_resolved = net::AddressResolver::resolve(custom_ip, 7777).toString();
    }
}

QString Tracker::getCustomIP()
{
    return custom_ip_resolved;
}

void Tracker::timedDelete(int ms)
{
    QTimer::singleShot(ms, this, &Tracker::deleteLater);
    connect(this, &Tracker::stopDone, this, &Tracker::deleteLater);
}

void Tracker::failed(const QString &err)
{
    error = err;
    status = TRACKER_ERROR;
    Q_EMIT requestFailed(err);
}

void Tracker::handleFailure()
{
    if (failureCount() > 5) {
        // we failed to contact the tracker 5 times in a row, so try again in
        // 30 minutes
        setInterval(FINAL_WAIT_TIME);
        reannounce_timer.start(FINAL_WAIT_TIME * 1000);
        request_time = QDateTime::currentDateTime();
    } else if (failureCount() > 2) {
        // we failed to contact the only tracker 3 times in a row, so try again in
        // a minute or 5, no need for hammering every 30 seconds
        setInterval(LONGER_WAIT_TIME);
        reannounce_timer.start(LONGER_WAIT_TIME * 1000);
        request_time = QDateTime::currentDateTime();
    } else {
        // lets not hammer and wait 30 seconds
        setInterval(INITIAL_WAIT_TIME);
        reannounce_timer.start(INITIAL_WAIT_TIME * 1000);
        request_time = QDateTime::currentDateTime();
    }
}

void Tracker::resetTrackerStats()
{
    bytes_downloaded_at_start = tds->bytesDownloaded();
    bytes_uploaded_at_start = tds->bytesUploaded();
}

Uint64 Tracker::bytesDownloaded() const
{
    Uint64 bd = tds->bytesDownloaded();
    if (bd > bytes_downloaded_at_start)
        return bd - bytes_downloaded_at_start;
    else
        return 0;
}

Uint64 Tracker::bytesUploaded() const
{
    Uint64 bu = tds->bytesUploaded();
    if (bu > bytes_uploaded_at_start)
        return bu - bytes_uploaded_at_start;
    else
        return 0;
}

}

#include "moc_tracker.cpp"
