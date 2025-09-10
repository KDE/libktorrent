/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPREALLOCATIONTHREAD_H
#define BTPREALLOCATIONTHREAD_H

#include "cachefile.h"
#include "ktorrent_export.h"
#include <QMap>
#include <QMutex>
#include <QString>
#include <QThread>
#include <util/constants.h>

namespace bt
{
/*!
 * \headerfile diskio/preallocationthread.h
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * \brief Thread to preallocate diskspace.
 */
class KTORRENT_EXPORT PreallocationThread : public QThread
{
public:
    PreallocationThread();
    ~PreallocationThread() override;

    //! Add a CacheFile to preallocate
    void add(CacheFile::Ptr cache_file);

    void run() override;

    /*!
     * Stop the thread.
     */
    void stop();

    /*!
     * Set an error message, also calls stop
     * \param msg The message
     */
    void setErrorMsg(const QString &msg);

    //! See if the thread has been stopped
    bool isStopped() const;

    //! Did an error occur during the preallocation ?
    bool errorHappened() const;

    //! Get the error_msg
    const QString &errorMessage() const
    {
        return error_msg;
    }

    //! nb Number of bytes have been written
    void written(Uint64 nb);

    //! Get the number of bytes written
    Uint64 bytesWritten();

    //! Allocation was aborted, so the next time the torrent is started it needs to be started again
    void setNotFinished();

    //! See if the allocation hasn't completed yet
    bool isNotFinished() const;

    //! See if the thread was done
    bool isDone() const;

private:
    bool expand(const QString &path, Uint64 max_size);

private:
    QList<CacheFile::Ptr> todo;
    bool stopped, not_finished, done;
    QString error_msg;
    Uint64 bytes_written;
    mutable QMutex mutex;
};

}

#endif
