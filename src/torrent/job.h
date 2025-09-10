/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_JOB_H
#define BT_JOB_H

#include "torrentstats.h"
#include <KIO/Job>
#include <ktorrent_export.h>

class KJobTrackerInterface;

namespace bt
{
class TorrentControl;

/*!
    \headerfile torrent/job.h
    \brief A KIO::Job which runs on a torrent.
*/
class KTORRENT_EXPORT Job : public KIO::Job
{
    Q_OBJECT
public:
    Job(bool stop_torrent, TorrentControl *tc);
    ~Job() override;

    //! Do we need to stop the torrent when the job is running
    [[nodiscard]] bool stopTorrent() const
    {
        return stop_torrent;
    }

    void start() override;
    virtual void kill(bool quietly = true);

    //! Return the status of the torrent during the job (default implementation returns INVALID_STATUS)
    [[nodiscard]] virtual TorrentStatus torrentStatus() const;

    //! Get the torrent associated with this job
    TorrentControl *torrent()
    {
        return tc;
    }

    //! Set the torrent associated with this job
    void setTorrent(TorrentControl *t)
    {
        tc = t;
    }

    //! Set the job tracker
    static void setJobTracker(KJobTrackerInterface *trk);

protected:
    //! Register the job with the tracker
    void registerWithTracker();

private:
    TorrentControl *tc;
    bool stop_torrent;

    static KJobTrackerInterface *tracker;
};

}

#endif // BT_JOB_H
