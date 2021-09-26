/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "trackerslist.h"
#include <torrent/torrent.h>

namespace bt
{
TrackersList::TrackersList()
{
}

TrackersList::~TrackersList()
{
}

void TrackersList::merge(const bt::TrackerTier *first)
{
    int tier = 1;
    while (first) {
        QList<QUrl>::const_iterator i = first->urls.begin();
        while (i != first->urls.end()) {
            addTracker(*i, true, tier);
            ++i;
        }
        tier++;
        first = first->next;
    }
}

}
