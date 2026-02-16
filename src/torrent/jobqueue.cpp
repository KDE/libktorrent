/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "jobqueue.h"
#include "job.h"
#include "torrentcontrol.h"
#include <util/log.h>

namespace bt
{
JobQueue::JobQueue(bt::TorrentControl *parent)
    : QObject(parent)
    , tc(parent)
    , restart(false)
{
}

JobQueue::~JobQueue()
{
    killAll();
}

void JobQueue::enqueue(Job *job)
{
    queue.append(job);
    if (queue.count() == 1) {
        startNextJob();
    }
}

bool JobQueue::runningJobs() const
{
    return queue.count() > 0;
}

Job *JobQueue::currentJob()
{
    return queue.isEmpty() ? nullptr : queue.front();
}

void JobQueue::startNextJob()
{
    if (queue.isEmpty()) {
        return;
    }

    Job *j = queue.front();
    connect(j, &Job::result, this, &JobQueue::jobDone);
    if (j->stopTorrent() && tc->getStats().running) {
        // stop the torrent if the job requires it
        tc->pause();
        restart = true;
    }
    j->start();
}

void JobQueue::jobDone(KJob *job)
{
    if (queue.isEmpty() || queue.front() != job) {
        return;
    }

    // remove the job and start the next
    queue.pop_front();
    if (!queue.isEmpty()) {
        startNextJob();
    } else {
        if (restart) {
            tc->unpause();
            tc->allJobsDone();
            restart = false;
        } else {
            tc->allJobsDone();
        }
    }
}

void JobQueue::killAll()
{
    if (queue.isEmpty()) {
        return;
    }

    queue.front()->kill();
    qDeleteAll(queue);
    queue.clear();
}

}
