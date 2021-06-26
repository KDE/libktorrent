/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
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
#include "movedatafilesjob.h"
#include <QFileInfo>
#include <QUrl>
#include <interfaces/torrentfileinterface.h>
#include <kio/jobuidelegate.h>
#include <kjobtrackerinterface.h>
#include <klocalizedstring.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
static ResourceManager move_data_files_slot(1);

MoveDataFilesJob::MoveDataFilesJob()
    : Job(true, nullptr)
    , Resource(&move_data_files_slot, "MoveDataFilesJob")
    , err(false)
    , active_job(nullptr)
    , running_recovery_jobs(0)
    , bytes_moved(0)
    , total_bytes(0)
    , bytes_moved_current_file(0)
{
}

MoveDataFilesJob::MoveDataFilesJob(const QMap<TorrentFileInterface *, QString> &fmap)
    : Job(true, nullptr)
    , Resource(&move_data_files_slot, "MoveDataFilesJob")
    , err(false)
    , active_job(nullptr)
    , running_recovery_jobs(0)
    , bytes_moved(0)
    , total_bytes(0)
    , bytes_moved_current_file(0)
{
    file_map = fmap;
    QMap<TorrentFileInterface *, QString>::const_iterator i = file_map.constBegin();
    while (i != file_map.constEnd()) {
        TorrentFileInterface *tf = i.key();
        QString dest = i.value();
        if (QFileInfo(dest).isDir()) {
            QString path = tf->getUserModifiedPath();
            if (!dest.endsWith(bt::DirSeparator()))
                dest += bt::DirSeparator();

            int last = path.lastIndexOf(bt::DirSeparator());
            QString dst = dest + path.mid(last + 1);
            if (QFileInfo(tf->getPathOnDisk()).canonicalPath() != QFileInfo(dst).canonicalPath())
                addMove(tf->getPathOnDisk(), dst);
        } else if (QFileInfo(tf->getPathOnDisk()).canonicalPath() != QFileInfo(i.value()).canonicalPath()) {
            addMove(tf->getPathOnDisk(), i.value());
        }
        ++i;
    }
}

MoveDataFilesJob::~MoveDataFilesJob()
{
}

void MoveDataFilesJob::addMove(const QString &src, const QString &dst)
{
    todo.insert(src, dst);
}

void MoveDataFilesJob::onJobDone(KJob *j)
{
    if (j->error() || err) {
        if (!err)
            setError(KIO::ERR_INTERNAL);

        active_job = nullptr;
        if (j->error())
            ((KIO::Job *)j)->uiDelegate()->showErrorMessage();

        // shit happened cancel all previous moves
        err = true;
        recover(j->error() != KIO::ERR_FILE_ALREADY_EXIST && j->error() != KIO::ERR_IDENTICAL_FILES);
    } else {
        bytes_moved += bytes_moved_current_file;
        bytes_moved_current_file = 0;
        success.insert(active_src, active_dst);
        active_src = active_dst = QString();
        active_job = nullptr;
        startMoving();
    }
}

void MoveDataFilesJob::start()
{
    registerWithTracker();
    QMap<QString, QString>::iterator i = todo.begin();
    while (i != todo.end()) {
        QFileInfo fi(i.key());
        total_bytes += fi.size();
        ++i;
    }
    setTotalAmount(KJob::Bytes, total_bytes);
    move_data_files_slot.add(this);
    if (!active_job) {
        description(this,
                    i18n("Waiting for other move jobs to finish"),
                    qMakePair(i18nc("The source of a file operation", "Source"), active_src),
                    qMakePair(i18nc("The destination of a file operation", "Destination"), active_dst));
        emitSpeed(0);
    }
}

void MoveDataFilesJob::acquired()
{
    startMoving();
}

void MoveDataFilesJob::kill(bool quietly)
{
    Q_UNUSED(quietly);
    // don't do anything we cannot abort in the middle of this operation
}

void MoveDataFilesJob::startMoving()
{
    if (todo.isEmpty()) {
        emitResult();
        return;
    }

    QMap<QString, QString>::iterator i = todo.begin();
    active_job = KIO::file_move(QUrl::fromLocalFile(i.key()), QUrl::fromLocalFile(i.value()), -1, KIO::HideProgressInfo);
    active_src = i.key();
    active_dst = i.value();
    Out(SYS_GEN | LOG_DEBUG) << "Moving " << active_src << " -> " << active_dst << endl;
    connect(active_job, &KIO::Job::result, this, &MoveDataFilesJob::onJobDone);
    connect(active_job, &KIO::Job::processedAmountChanged, this, &MoveDataFilesJob::onTransferred);
    connect(active_job, &KIO::Job::speed, this, &MoveDataFilesJob::onSpeed);
    todo.erase(i);

    description(this,
                i18nc("@title job", "Moving"),
                qMakePair(i18nc("The source of a file operation", "Source"), active_src),
                qMakePair(i18nc("The destination of a file operation", "Destination"), active_dst));
    addSubjob(active_job);
}

void MoveDataFilesJob::recover(bool delete_active)
{
    if (delete_active && bt::Exists(active_dst))
        bt::Delete(active_dst, true);

    if (success.isEmpty()) {
        emitResult();
        return;
    }

    running_recovery_jobs = 0;
    QMap<QString, QString>::iterator i = success.begin();
    while (i != success.end()) {
        KIO::Job *j = KIO::file_move(QUrl::fromLocalFile(i.value()), QUrl::fromLocalFile(i.key()), -1, KIO::HideProgressInfo);
        connect(j, &KIO::Job::result, this, &MoveDataFilesJob::onRecoveryJobDone);
        running_recovery_jobs++;
        ++i;
    }
    success.clear();
}

void MoveDataFilesJob::onRecoveryJobDone(KJob *j)
{
    Q_UNUSED(j);
    running_recovery_jobs--;
    if (running_recovery_jobs <= 0)
        emitResult();
}

void MoveDataFilesJob::onTransferred(KJob *job, KJob::Unit unit, qulonglong amount)
{
    Q_UNUSED(job);
    if (unit == KJob::Bytes) {
        bytes_moved_current_file = amount;
        setProcessedAmount(unit, bytes_moved + bytes_moved_current_file);
    }
}

void MoveDataFilesJob::onSpeed(KJob *job, unsigned long speed)
{
    Q_UNUSED(job);
    emitSpeed(speed);
}

}
