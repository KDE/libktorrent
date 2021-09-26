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
#ifndef BT_PREALLOCATIONJOB_H
#define BT_PREALLOCATIONJOB_H

#include <torrent/job.h>

namespace bt
{
class PreallocationThread;
class ChunkManager;

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
