/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "extractfilejob.h"
#include <KIO/Global>
#include <QFile>
#include <QThread>

namespace bt
{
class ExtractFileThread : public QThread
{
public:
    ExtractFileThread(QIODevice *in_dev, QIODevice *out_dev)
        : in_dev(in_dev)
        , out_dev(out_dev)
        , canceled(false)
    {
    }

    ~ExtractFileThread() override
    {
        delete in_dev;
        delete out_dev;
    }

    void run() override
    {
        char buf[4096];
        qint64 ret = 0;
        while ((ret = in_dev->read(buf, 4096)) != 0 && !canceled) {
            out_dev->write(buf, ret);
        }
    }

    QIODevice *in_dev;
    QIODevice *out_dev;
    bool canceled;
};

ExtractFileJob::ExtractFileJob(KArchive *archive, const QString &path, const QString &dest)
    : archive(archive)
    , path(path)
    , dest(dest)
    , extract_thread(nullptr)
{
}

ExtractFileJob::~ExtractFileJob()
{
    delete archive;
}

void ExtractFileJob::start()
{
    // first find the file in the archive
    QStringList path_components = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    const KArchiveDirectory *dir = archive->directory();
    for (int i = 0; i < path_components.count(); i++) {
        // if we can't find it give back an error
        QString pc = path_components.at(i);
        if (!dir->entries().contains(pc)) {
            setError(KIO::ERR_DOES_NOT_EXIST);
            emitResult();
            return;
        }

        const KArchiveEntry *e = dir->entry(pc);
        if (i < path_components.count() - 1) {
            // if we are not the last entry in the path, e must be a directory
            if (!e->isDirectory()) {
                setError(KIO::ERR_DOES_NOT_EXIST);
                emitResult();
                return;
            }

            dir = (const KArchiveDirectory *)e;
        } else {
            // last in the path, must be a file
            if (!e->isFile()) {
                setError(KIO::ERR_DOES_NOT_EXIST);
                emitResult();
                return;
            }

            // create a device to read the file and start a thread to do the reading
            KArchiveFile *file = (KArchiveFile *)e;
            QFile *out_dev = new QFile(dest);
            if (!out_dev->open(QIODevice::WriteOnly)) {
                delete out_dev;
                setError(KIO::ERR_CANNOT_OPEN_FOR_WRITING);
                emitResult();
                return;
            }

            QIODevice *in_dev = file->createDevice();
            extract_thread = new ExtractFileThread(in_dev, out_dev);
            connect(extract_thread, &ExtractFileThread::finished, this, &ExtractFileJob::extractThreadDone, Qt::QueuedConnection);
            extract_thread->start();
        }
    }
}

void ExtractFileJob::kill(bool quietly)
{
    if (extract_thread) {
        extract_thread->canceled = true;
        extract_thread->wait();
        delete extract_thread;
        extract_thread = nullptr;
    }
    setError(KIO::ERR_USER_CANCELED);
    if (!quietly)
        emitResult();
}

void ExtractFileJob::extractThreadDone()
{
    extract_thread->wait();
    delete extract_thread;
    extract_thread = nullptr;
    setError(0);
    emitResult();
}

}
