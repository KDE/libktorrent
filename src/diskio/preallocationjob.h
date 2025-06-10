/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BT_PREALLOCATIONJOB_H
#define BT_PREALLOCATIONJOB_H

#include <torrent/job.h>

namespace bt
{
class PreallocationThread;
class ChunkManager;

/*!
 * \brief Job to preallocates disk space for a torrent.
 */
class KTORRENT_EXPORT PreallocationJob : public bt::Job
{
    Q_OBJECT
public:
    PreallocationJob(ChunkManager *cman, TorrentControl *tc);
    ~PreallocationJob() override;

    void start() override;
    void kill(bool quietly = true) override;
    TorrentStatus torrentStatus() const override
    {
        return ALLOCATING_DISKSPACE;
    }

private Q_SLOTS:
    void finished();

private:
    ChunkManager *cman;
    PreallocationThread *prealloc_thread;
};

}

#endif // BT_PREALLOCATIONJOB_H
