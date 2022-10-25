/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "job.h"
#include "torrentcontrol.h"
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KJobTrackerInterface>
#include <kio_version.h>

namespace bt
{
KJobTrackerInterface *Job::tracker = nullptr;

Job::Job(bool stop_torrent, bt::TorrentControl *tc)
    : tc(tc)
    , stop_torrent(stop_torrent)
{
#if KIO_VERSION < QT_VERSION_CHECK(5, 98, 0)
    setUiDelegate(new KIO::JobUiDelegate());
#else
    setUiDelegate(KIO::createDefaultJobUiDelegate());
#endif
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
