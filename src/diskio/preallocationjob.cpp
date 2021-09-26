/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "preallocationjob.h"
#include "chunkmanager.h"
#include "preallocationthread.h"
#include <torrent/torrentcontrol.h>
#include <util/log.h>

namespace bt
{
PreallocationJob::PreallocationJob(ChunkManager *cman, TorrentControl *tc)
    : Job(false, tc)
    , cman(cman)
    , prealloc_thread(nullptr)
{
}

PreallocationJob::~PreallocationJob()
{
}

void PreallocationJob::start()
{
    prealloc_thread = new PreallocationThread();
    cman->preparePreallocation(prealloc_thread);
    connect(prealloc_thread, &PreallocationThread::finished, this, &PreallocationJob::finished, Qt::QueuedConnection);
    prealloc_thread->start(QThread::IdlePriority);
}

void PreallocationJob::kill(bool quietly)
{
    if (prealloc_thread) {
        prealloc_thread->stop();
        prealloc_thread->wait();
        prealloc_thread->deleteLater();
        prealloc_thread = nullptr;
    }
    bt::Job::kill(quietly);
}

void PreallocationJob::finished()
{
    if (prealloc_thread) {
        torrent()->preallocFinished(prealloc_thread->errorMessage(), !prealloc_thread->isStopped());
        prealloc_thread->deleteLater();
        prealloc_thread = nullptr;
    } else
        torrent()->preallocFinished(QString(), false);

    setError(0);
    emitResult();
}

}
