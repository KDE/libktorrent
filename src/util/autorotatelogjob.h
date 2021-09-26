/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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
