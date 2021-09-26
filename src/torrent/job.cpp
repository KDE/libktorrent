/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "job.h"
#include "torrentcontrol.h"
#include <KIO/JobUiDelegate>
#include <KJobTrackerInterface>

namespace bt
{
KJobTrackerInterface *Job::tracker = nullptr;

Job::Job(bool stop_torrent, bt::TorrentControl *tc)
    : tc(tc)
    , stop_torrent(stop_torrent)
{
    setUiDelegate(new KIO::JobUiDelegate());
}

Job::~Job()
{
}

void Job::start()
{
}

void Job::kill(bool quietly)
{
    if (!quietly) {
        setError(KIO::ERR_USER_CANCELED);
        emitResult();
    }
}

TorrentStatus Job::torrentStatus() const
{
    return INVALID_STATUS;
}

void Job::setJobTracker(KJobTrackerInterface *trk)
{
    tracker = trk;
}

void Job::registerWithTracker()
{
    if (tracker)
        tracker->registerJob(this);
}

}
