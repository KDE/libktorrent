/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTWAITJOB_H
#define BTWAITJOB_H

#include <KIO/Job>
#include <QList>

#include "constants.h"
#include <interfaces/exitoperation.h>
#include <ktorrent_export.h>

namespace bt
{
/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Job to wait for a certain amount of time or until one or more ExitOperation's have
 * finished.
 */
class KTORRENT_EXPORT WaitJob : public KIO::Job
{
public:
    WaitJob(Uint32 millis);
    ~WaitJob() override;

    virtual void kill(bool quietly = true);

    /**
     * Add an ExitOperation;
     * @param op The operation
     */
    void addExitOperation(ExitOperation *op);

    /**
     * Add a KIO::Job to wait on;
     * @param job The job
     */
    void addExitOperation(KIO::Job *job);

    /**
     * Execute a WaitJob
     * @param job The Job
     */
    static void execute(WaitJob *job);

    /// Are there any ExitOperation's we need to wait for
    bool needToWait() const
    {
        return exit_ops.count() > 0;
    }

private:
    void timerDone();
    void operationFinished(ExitOperation *op);

private:
    QList<ExitOperation *> exit_ops;
};

KTORRENT_EXPORT void SynchronousWait(Uint32 millis);

}

#endif
