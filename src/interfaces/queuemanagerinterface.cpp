/*
    SPDX-FileCopyrightText: 2007 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "queuemanagerinterface.h"
#include <torrent/torrentcontrol.h>

namespace bt
{
bool QueueManagerInterface::qm_enabled = true;

QueueManagerInterface::QueueManagerInterface()
{
}

QueueManagerInterface::~QueueManagerInterface()
{
}

void QueueManagerInterface::setQueueManagerEnabled(bool on)
{
    qm_enabled = on;
}

bool QueueManagerInterface::permitStatsSync(TorrentControl *tc)
{
    return tc->getStatsSyncElapsedTime() >= 5 * 60 * 1000; // 5 sec
}

}
