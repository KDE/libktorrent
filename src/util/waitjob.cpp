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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
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
