/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "trackerinterface.h"
#include <klocalizedstring.h>

namespace bt
{
TrackerInterface::TrackerInterface(const QUrl &url)
    : url(url)
{
    // default 5 minute interval
    interval = 5 * 60 * 1000;
    seeders = leechers = total_downloaded = -1;
    enabled = true;
    started = false;
    status = TRACKER_IDLE;
    time_out = false;
}

TrackerInterface::~TrackerInterface()
{
}

void TrackerInterface::reset()
{
    interval = 5 * 60 * 1000;
    status = TRACKER_IDLE;
}

Uint32 TrackerInterface::timeToNextUpdate() const
{
    if (!enabled || !isStarted())
        return 0;
    else
        return interval - request_time.secsTo(QDateTime::currentDateTime());
}

QString TrackerInterface::trackerStatusString() const
{
    switch (status) {
    case TRACKER_OK:
        return warning.isEmpty() ? i18n("OK") : i18n("Warning: %1", warning);
    case TRACKER_ANNOUNCING:
        return i18n("Announcing");
    case TRACKER_ERROR:
        return i18n("Error: %1", error);
    case TRACKER_IDLE:
    default:
        return QString();
    }
}

}
