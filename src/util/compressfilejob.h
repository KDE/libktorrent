/*
    SPDX-FileCopyrightText: 2008 Joris Guisson and Ivan Vasic
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
#ifndef BTCOMPRESSFILEJOB_H
#define BTCOMPRESSFILEJOB_H

#include <KIO/Job>
#include <QThread>
#include <ktorrent_export.h>

namespace bt
{
class KTORRENT_EXPORT CompressThread : public QThread
{
public:
    CompressThread(const QString &file);
    ~CompressThread() override;

    /// Run the compression thread
    void run() override;

    /// Cancel the thread, things should be cleaned up properly
    void cancel();

    /// Get the error which happened (0 means no error)
    int error() const
    {
        return err;
    }

private:
    QString file;
    bool canceled;
    int err;
};

/**
    Compress a file using gzip and remove it when completed successfully.
*/
class KTORRENT_EXPORT CompressFileJob : public KIO::Job
{
public:
    CompressFileJob(const QString &file);
    ~CompressFileJob() override;

    void start() override;
    virtual void kill(bool quietly = true);

private:
    void compressThreadFinished();

private:
    QString file;
    CompressThread *compress_thread;
};

}

#endif
