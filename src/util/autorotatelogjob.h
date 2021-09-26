/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

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
#ifndef BTAUTOROTATELOGJOB_H
#define BTAUTOROTATELOGJOB_H

#include <cstdlib>
#include <kio/job.h>

namespace bt
{
class Log;

/**
    @author Joris Guisson <joris.guisson@gmail.com>

    Job which handles the rotation of the log file.
    This Job must do several move jobs which must be done sequentially.
*/
class AutoRotateLogJob : public KIO::Job
{
public:
    AutoRotateLogJob(const QString &file, Log *lg);
    ~AutoRotateLogJob() override;

    virtual void kill(bool quietly = true);

private:
    void moveJobDone(KJob *);
    void compressJobDone(KJob *);

    void update();

private:
    QString file;
    int cnt;
    Log *lg;
};

}

#endif
