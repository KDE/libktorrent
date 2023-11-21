/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "preallocationthread.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "chunkmanager.h"
#include <klocalizedstring.h>
#include <qfile.h>
#include <util/error.h>
#include <util/log.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

namespace bt
{
PreallocationThread::PreallocationThread()
    : stopped(false)
    , not_finished(false)
    , done(false)
    , bytes_written(0)
{
}

PreallocationThread::~PreallocationThread()
{
}

void PreallocationThread::add(CacheFile::Ptr cache_file)
{
    if (cache_file)
        todo.append(cache_file);
}

void PreallocationThread::run()
{
    try {
        for (CacheFile::Ptr cache_file : std::as_const(todo)) {
            if (!isStopped()) {
                cache_file->preallocate(this);
            } else {
                setNotFinished();
                break;
            }
        }

    } catch (Error &err) {
        setErrorMsg(err.toString());
    }

    QMutexLocker lock(&mutex);
    done = true;
    Out(SYS_DIO | LOG_NOTICE) << "PreallocationThread has finished" << endl;
}

void PreallocationThread::stop()
{
    QMutexLocker lock(&mutex);
    stopped = true;
}

void PreallocationThread::setErrorMsg(const QString &msg)
{
    QMutexLocker lock(&mutex);
    error_msg = msg;
    stopped = true;
}

bool PreallocationThread::isStopped() const
{
    QMutexLocker lock(&mutex);
    return stopped;
}

bool PreallocationThread::errorHappened() const
{
    QMutexLocker lock(&mutex);
    return !error_msg.isNull();
}

void PreallocationThread::written(Uint64 nb)
{
    QMutexLocker lock(&mutex);
    bytes_written += nb;
}

Uint64 PreallocationThread::bytesWritten()
{
    QMutexLocker lock(&mutex);
    return bytes_written;
}

bool PreallocationThread::isDone() const
{
    QMutexLocker lock(&mutex);
    return done;
}

bool PreallocationThread::isNotFinished() const
{
    QMutexLocker lock(&mutex);
    return not_finished;
}

void PreallocationThread::setNotFinished()
{
    QMutexLocker lock(&mutex);
    not_finished = true;
}
}
