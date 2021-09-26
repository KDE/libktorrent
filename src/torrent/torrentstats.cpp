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
#include "torrentstats.h"
#include <klocalizedstring.h>
#include <util/functions.h>

namespace bt
{
TorrentStats::TorrentStats()
    : imported_bytes(0)
    , bytes_downloaded(0)
    , bytes_uploaded(0)
    , bytes_left(0)
    , bytes_left_to_download(0)
    , total_bytes(0)
    , total_bytes_to_download(0)
    , download_rate(0)
    , upload_rate(0)
    , num_peers(0)
    , num_chunks_downloading(0)
    , total_chunks(0)
    , num_chunks_downloaded(0)
    , num_chunks_excluded(0)
    , num_chunks_left(0)
    , chunk_size(0)
    , seeders_total(0)
    , seeders_connected_to(0)
    , leechers_total(0)
    , leechers_connected_to(0)
    , status(NOT_STARTED)
    , session_bytes_downloaded(0)
    , session_bytes_uploaded(0)
    , running(false)
    , started(false)
    , queued(false)
    , autostart(false)
    , stopped_by_error(false)
    , completed(false)
    , paused(false)
    , auto_stopped(false)
    , superseeding(false)
    , qm_can_start(false)
    , multi_file_torrent(false)
    , priv_torrent(false)
    , max_share_ratio(0.0f)
    , max_seed_time(0.0f)
    , num_corrupted_chunks(0)
{
    last_download_activity_time = last_upload_activity_time = bt::CurrentTime();
}

float TorrentStats::shareRatio() const
{
    if (bytes_downloaded == 0)
        return 0.0f;
    else
        return (float)bytes_uploaded / (bytes_downloaded /*+ stats.imported_bytes*/);
}

bool TorrentStats::overMaxRatio() const
{
    return (completed && max_share_ratio > 0) && (shareRatio() - max_share_ratio > 0.00001);
}

QString TorrentStats::statusToString() const
{
    switch (status) {
    case NOT_STARTED:
        return i18n("Not started");
    case DOWNLOAD_COMPLETE:
        return i18n("Download completed");
    case SEEDING_COMPLETE:
        return i18n("Seeding completed");
    case SEEDING:
        return i18nc("Status of a torrent file", "Seeding");
    case DOWNLOADING:
        return i18n("Downloading");
    case STALLED:
        return i18n("Stalled");
    case STOPPED:
        return i18n("Stopped");
    case ERROR:
        return i18n("Error: %1", error_msg);
    case ALLOCATING_DISKSPACE:
        return i18n("Allocating diskspace");
    case QUEUED:
        return completed ? i18n("Queued for seeding") : i18n("Queued for downloading");
    case CHECKING_DATA:
        return i18n("Checking data");
    case NO_SPACE_LEFT:
        return i18n("Stopped. No space left on device.");
    case PAUSED:
        return i18n("Paused");
    case SUPERSEEDING:
        return i18n("Superseeding");
    default:
        return QString();
    }
}
}
