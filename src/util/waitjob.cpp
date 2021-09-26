/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "waitjob.h"
#include "log.h"
#include <qtimer.h>
#include <torrent/globals.h>

namespace bt
{
WaitJob::WaitJob(Uint32 millis)
    : KIO::Job(/*false*/)
{
    QTimer::singleShot(millis, this, &WaitJob::timerDone);
}

WaitJob::~WaitJob()
{
    for (ExitOperation *op : qAsConst(exit_ops))
        delete op;
}

void WaitJob::kill(bool)
{
    emitResult();
}

void WaitJob::timerDone()
{
    emitResult();
}

void WaitJob::addExitOperation(ExitOperation *op)
{
    exit_ops.append(op);
    connect(op, &ExitOperation::operationFinished, this, &WaitJob::operationFinished);
}

void WaitJob::addExitOperation(KIO::Job *job)
{
    addExitOperation(new ExitJobOperation(job));
}

void WaitJob::operationFinished(ExitOperation *op)
{
    if (exit_ops.count() > 0) {
        exit_ops.removeAll(op);
        if (op->deleteAllowed())
            op->deleteLater();

        if (exit_ops.count() == 0)
            timerDone();
    }
}

void WaitJob::execute(WaitJob *job)
{
    job->exec();
}

void SynchronousWait(Uint32 millis)
{
    Out(SYS_GEN | LOG_DEBUG) << "SynchronousWait" << endl;
    WaitJob *j = new WaitJob(millis);
    j->exec();
}

}
