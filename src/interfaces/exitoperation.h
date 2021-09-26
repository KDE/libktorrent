/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTEXITOPERATION_H
#define BTEXITOPERATION_H

#include <kio/job.h>
#include <ktorrent_export.h>
#include <qobject.h>

namespace bt
{
/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Object to derive from for operations which need to be performed at exit.
 * The operation should emit the operationFinished signal when they are done.
 *
 * ExitOperation's can be used in combination with a WaitJob, to wait for a certain amount of time
 * to give several ExitOperation's the time time to finish up.
 */
class KTORRENT_EXPORT ExitOperation : public QObject
{
    Q_OBJECT
public:
    ExitOperation();
    ~ExitOperation() override;

    /// whether or not we can do a deleteLater on the job after it has finished.
    virtual bool deleteAllowed() const
    {
        return true;
    }
Q_SIGNALS:
    void operationFinished(ExitOperation *opt);
};

/**
 * Exit operation which waits for a KIO::Job
 */
class ExitJobOperation : public ExitOperation
{
public:
    ExitJobOperation(KJob *j);
    ~ExitJobOperation() override;

    bool deleteAllowed() const override
    {
        return true;
    }

private:
    virtual void onResult(KJob *j);
};
}

#endif
