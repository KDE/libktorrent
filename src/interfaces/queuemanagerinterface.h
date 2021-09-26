/*
    SPDX-FileCopyrightText: 2007 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

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
#ifndef BTQUEUEMANAGERINTERFACE_H
#define BTQUEUEMANAGERINTERFACE_H

#include <ktorrent_export.h>

namespace bt
{
class SHA1Hash;
class TorrentControl;
struct TrackerTier;

/**
    @author
*/
class KTORRENT_EXPORT QueueManagerInterface
{
    static bool qm_enabled;

public:
    QueueManagerInterface();
    virtual ~QueueManagerInterface();

    /**
     * See if we already loaded a torrent.
     * @param ih The info hash of a torrent
     * @return true if we do, false if we don't
     */
    virtual bool alreadyLoaded(const SHA1Hash &ih) const = 0;

    /**
     * Merge announce lists to a torrent
     * @param ih The info_hash of the torrent to merge to
     * @param trk First tier of trackers
     */
    virtual void mergeAnnounceList(const SHA1Hash &ih, const TrackerTier *trk) = 0;

    /**
     * Disable or enable the QM
     * @param on
     */
    static void setQueueManagerEnabled(bool on);

    /**
     * Requested by each TorrentControl during its update to
     * get permission on saving Stats file to disk. May be
     * overriden to balance I/O operations.
     * @param tc Pointer to TorrentControl instance
     * @return true if file save is permitted, false otherwise
     */

    virtual bool permitStatsSync(TorrentControl *tc);

    static bool enabled()
    {
        return qm_enabled;
    }
};

}

#endif
