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

#ifndef BT_JOBQUEUE_H
#define BT_JOBQUEUE_H

#include <QObject>
#include <ktorrent_export.h>
#include <torrent/job.h>

namespace bt
{
class Job;

/**
    A job queue handles all jobs running on a torrent in a sequential order
*/
class KTORRENT_EXPORT JobQueue : public QObject
{
public:
    JobQueue(TorrentControl *parent);
    ~JobQueue() override;

    /// Are there running jobs
    bool runningJobs() const;

    /// Start the next job
    void startNextJob();

    /// Enqueue a job
    void enqueue(Job *job);

    /// Get the current job
    Job *currentJob();

    /// Kill all jobs
    void killAll();

private:
    void jobDone(KJob *job);

    QList<Job *> queue;
    TorrentControl *tc;
    bool restart;
};

}

#endif // BT_JOBQUEUE_H
