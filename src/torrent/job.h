/*
    SPDX-FileCopyrightText: 2009 by
    Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

/**
    A Job is a KIO::Job which runs on a torrent
*/
class KTORRENT_EXPORT Job : public KIO::Job
{
    Q_OBJECT
public:
    Job(bool stop_torrent, TorrentControl *tc);
    ~Job() override;

    /// Do we need to stop the torrent when the job is running
    bool stopTorrent() const
    {
        return stop_torrent;
    }

    void start() override;
    virtual void kill(bool quietly = true);

    /// Return the status of the torrent during the job (default implementation returns INVALID_STATUS)
    virtual TorrentStatus torrentStatus() const;

    /// Get the torrent associated with this job
    TorrentControl *torrent()
    {
        return tc;
    }

    /// Set the torrent associated with this job
    void setTorrent(TorrentControl *t)
    {
        tc = t;
    }

    /// Set the job tracker
    static void setJobTracker(KJobTrackerInterface *trk);

protected:
    /// Register the job with the tracker
    void registerWithTracker();

private:
    TorrentControl *tc;
    bool stop_torrent;

    static KJobTrackerInterface *tracker;
};

}

#endif // BT_JOB_H
