/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_JOBQUEUE_H
#define BT_JOBQUEUE_H

#include <QObject>
#include <ktorrent_export.h>
#include <torrent/job.h>

namespace bt
{
class Job;

/*!
 * \brief Handles all jobs running on a torrent in a sequential order.
 */
class KTORRENT_EXPORT JobQueue : public QObject
{
public:
    JobQueue(TorrentControl *parent);
    ~JobQueue() override;

    //! Are there running jobs
    bool runningJobs() const;

    //! Start the next job
    void startNextJob();

    //! Enqueue a job
    void enqueue(Job *job);

    //! Get the current job
    Job *currentJob();

    //! Kill all jobs
    void killAll();

private:
    void jobDone(KJob *job);

    QList<Job *> queue;
    TorrentControl *tc;
    bool restart;
};

}

#endif // BT_JOBQUEUE_H
