/*
    SPDX-FileCopyrightText: 2008 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
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
