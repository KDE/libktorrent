/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "autorotatelogjob.h"

#include <KIO/FileCopyJob>
#include <QUrl>

#include "compressfilejob.h"
#include "log.h"
#include <util/fileops.h>

namespace bt
{
AutoRotateLogJob::AutoRotateLogJob(const QString &file, Log *lg)
    : KIO::Job()
    , file(file)
    , cnt(10)
    , lg(lg)
{
    update();
}

AutoRotateLogJob::~AutoRotateLogJob()
{
}

void AutoRotateLogJob::kill(bool)
{
    emitResult();
}

void AutoRotateLogJob::update()
{
    while (cnt > 1) {
        QString prev = QStringLiteral("%1-%2.gz").arg(file).arg(cnt - 1);
        QString curr = QStringLiteral("%1-%2.gz").arg(file).arg(cnt);
        if (bt::Exists(prev)) { // if file exists start the move job
            KIO::Job *sj = KIO::file_move(QUrl::fromLocalFile(prev), QUrl::fromLocalFile(curr), -1, KIO::Overwrite | KIO::HideProgressInfo);
            connect(sj, &KIO::Job::result, this, &AutoRotateLogJob::moveJobDone);
            return;
        } else {
            cnt--;
        }
    }

    if (cnt == 1) {
        // move current log to 1 and zip it
        KIO::Job *sj = KIO::file_move(QUrl::fromLocalFile(file), QUrl::fromLocalFile(file + QLatin1String("-1")), -1, KIO::Overwrite | KIO::HideProgressInfo);
        connect(sj, &KIO::Job::result, this, &AutoRotateLogJob::moveJobDone);
    } else {
        // final log file is moved, now zip it and end the job
        CompressFileJob *gzip = new CompressFileJob(file + QLatin1String("-1"));
        connect(gzip, &CompressFileJob::result, this, &AutoRotateLogJob::compressJobDone);
        gzip->start();
    }
}

void AutoRotateLogJob::moveJobDone(KJob *)
{
    cnt--; // decrease counter so the new file will be moved in update
    update(); // don't care about result of job
}

void AutoRotateLogJob::compressJobDone(KJob *)
{
    lg->logRotateDone();
    emitResult();
}

}
