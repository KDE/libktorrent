/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTMOVEDATAFILESJOB_H
#define BTMOVEDATAFILESJOB_H

#include <torrent/job.h>
#include <util/resourcemanager.h>

namespace bt
{
class TorrentFileInterface;

/*!
 * \headerfile diskio/movedatafilesjob.h
 * \author Joris Guisson <joris.guisson@gmail.com>
 * \brief Job to move all files of a torrent.
 */
class MoveDataFilesJob : public Job, public Resource
{
public:
    MoveDataFilesJob();

    /*!
        Constructor with a file map.
        \param fmap Map of files and their destinations
    */
    MoveDataFilesJob(const QMap<TorrentFileInterface *, QString> &fmap);
    ~MoveDataFilesJob() override;

    /*!
     * Add a move to the todo list.
     * \param src File to move
     * \param dst Where to move it to
     */
    void addMove(const QString &src, const QString &dst);

    void start() override;
    void kill(bool quietly = true) override;

    //! Get the file map (could be empty)
    [[nodiscard]] const QMap<TorrentFileInterface *, QString> &fileMap() const
    {
        return file_map;
    }

private:
    void onJobDone(KJob *j);
    void onRecoveryJobDone(KJob *j);
    void onTransferred(KJob *job, KJob::Unit unit, qulonglong amount);
    void onSpeed(KJob *job, unsigned long speed);

    void recover(bool delete_active);
    void startMoving();
    void acquired() override;

private:
    bool err;
    KIO::Job *active_job;
    QString active_src, active_dst;
    QMap<QString, QString> todo;
    QMap<QString, QString> success;
    int running_recovery_jobs;
    QMap<TorrentFileInterface *, QString> file_map;

    bt::Uint64 bytes_moved;
    bt::Uint64 total_bytes;
    bt::Uint64 bytes_moved_current_file;
};

}

#endif
