/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DECOMPRESSFILEJOB_H
#define DECOMPRESSFILEJOB_H

#include <KIO/Job>
#include <QThread>
#include <ktorrent_export.h>

namespace bt
{
/*!
 * Thread which decompresses a single file
 */
class KTORRENT_EXPORT DecompressThread : public QThread
{
public:
    DecompressThread(const QString &file, const QString &dest_file);
    ~DecompressThread() override;

    //! Run the decompression thread
    void run() override;

    //! Cancel the thread, things should be cleaned up properly
    void cancel();

    //! Get the error which happened (0 means no error)
    int error() const
    {
        return err;
    }

private:
    QString file;
    QString dest_file;
    bool canceled;
    int err;
};

/*!
    Decompress a file and remove it when completed successfully.
*/
class KTORRENT_EXPORT DecompressFileJob : public KIO::Job
{
    Q_OBJECT
public:
    DecompressFileJob(const QString &file, const QString &dest);
    ~DecompressFileJob() override;

    void start() override;
    virtual void kill(bool quietly = true);

private Q_SLOTS:
    void decompressThreadFinished();

private:
    QString file;
    QString dest;
    DecompressThread *decompress_thread;
};
}

#endif // DECOMPRESSFILEJOB_H
