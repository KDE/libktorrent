/***************************************************************************
 *   Copyright (C) 2009 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

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
