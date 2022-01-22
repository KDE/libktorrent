/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "decompressfilejob.h"

#include <QFile>
#include <QMimeDatabase>

#include <KCompressionDevice>

#include <util/fileops.h>
#include <util/log.h>

namespace bt
{
DecompressThread::DecompressThread(const QString &file, const QString &dest_file)
    : file(file)
    , dest_file(dest_file)
    , canceled(false)
    , err(0)
{
}

DecompressThread::~DecompressThread()
{
}

void DecompressThread::run()
{
    QFile out(dest_file);

    // open input file readonly
    if (!out.open(QIODevice::WriteOnly)) {
        err = KIO::ERR_CANNOT_OPEN_FOR_WRITING;
        Out(SYS_GEN | LOG_NOTICE) << "Failed to open " << dest_file << " : " << out.errorString() << endl;
        return;
    }

    // open output file
    KCompressionDevice dev(file, KCompressionDevice::compressionTypeForMimeType(QMimeDatabase().mimeTypeForFile(file).name()));
    if (!dev.open(QIODevice::ReadOnly)) {
        err = KIO::ERR_CANNOT_OPEN_FOR_READING;
        Out(SYS_GEN | LOG_NOTICE) << "Failed to open " << file << " : " << dev.errorString() << endl;
        return;
    }

    // copy the data
    char buf[4096];
    while (!canceled && !dev.atEnd()) {
        int len = dev.read(buf, 4096);
        if (len <= 0 || len > 4096)
            break;

        out.write(buf, len);
    }

    out.close();
    if (canceled) {
        // delete output file when canceled
        bt::Delete(dest_file, true);
    } else {
        // delete the input file upon success
        bt::Delete(file, true);
    }
}

void DecompressThread::cancel()
{
    canceled = true;
}

///////////////////////////////////////////

DecompressFileJob::DecompressFileJob(const QString &file, const QString &dest)
    : file(file)
    , dest(dest)
    , decompress_thread(nullptr)
{
}

DecompressFileJob::~DecompressFileJob()
{
}

void DecompressFileJob::start()
{
    decompress_thread = new DecompressThread(file, dest);
    connect(decompress_thread, &DecompressThread::finished, this, &DecompressFileJob::decompressThreadFinished, Qt::QueuedConnection);
    decompress_thread->start();
}

void DecompressFileJob::kill(bool quietly)
{
    if (decompress_thread) {
        decompress_thread->cancel();
        decompress_thread->wait();
        delete decompress_thread;
        decompress_thread = nullptr;
    }
    setError(KIO::ERR_USER_CANCELED);
    if (!quietly)
        emitResult();
}

void DecompressFileJob::decompressThreadFinished()
{
    setError(decompress_thread->error());
    decompress_thread->wait();
    delete decompress_thread;
    decompress_thread = nullptr;
    emitResult();
}

}
