/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "compressfilejob.h"

#include <QFile>
#include <QThread>

#include <KFilterDev>
#include <KIO/Global>

#include "fileops.h"

namespace bt
{
CompressThread::CompressThread(const QString &file)
    : file(file)
    , canceled(false)
    , err(0)
{
}

CompressThread::~CompressThread()
{
}

void CompressThread::run()
{
    QFile in(file);

    // open input file readonly
    if (!in.open(QIODevice::ReadOnly)) {
        err = KIO::ERR_CANNOT_OPEN_FOR_READING;
        printf("CompressThread: failed to open input file %s for reading: %s\n",
               in.fileName().toLocal8Bit().constData(),
               in.errorString().toLocal8Bit().constData());
        return;
    }

    // open output file
    KCompressionDevice dev(file + QLatin1String(".gz"), KCompressionDevice::GZip);
    if (!dev.open(QIODevice::WriteOnly)) {
        err = KIO::ERR_CANNOT_OPEN_FOR_WRITING;
        printf("CompressThread: failed to open out file for writing");
        return;
    }

    // copy the data
    char buf[4096];
    while (!canceled && !in.atEnd()) {
        int len = in.read(buf, 4096);
        if (len <= 0 || len > 4096)
            break;

        dev.write(buf, len);
    }

    in.close();
    if (canceled) {
        // delete output file when canceled
        bt::Delete(file + QLatin1String(".gz"), true);
    } else {
        // delete the input file upon success
        bt::Delete(file, true);
    }
}

void CompressThread::cancel()
{
    canceled = true;
}

////////////////////////////////////////////////////////////

CompressFileJob::CompressFileJob(const QString &file)
    : file(file)
    , compress_thread(nullptr)
{
}

CompressFileJob::~CompressFileJob()
{
}

void CompressFileJob::start()
{
    compress_thread = new CompressThread(file);
    connect(compress_thread, &CompressThread::finished, this, &CompressFileJob::compressThreadFinished, Qt::QueuedConnection);
    compress_thread->start();
}

void CompressFileJob::kill(bool quietly)
{
    if (compress_thread) {
        compress_thread->cancel();
        compress_thread->wait();
        delete compress_thread;
        compress_thread = nullptr;
    }
    setError(KIO::ERR_USER_CANCELED);
    if (!quietly)
        emitResult();
}

void CompressFileJob::compressThreadFinished()
{
    setError(compress_thread->error());
    compress_thread->wait();
    delete compress_thread;
    compress_thread = nullptr;
    emitResult();
}

}
