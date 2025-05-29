/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_EXTRACTFILEJOB_H
#define BT_EXTRACTFILEJOB_H

#include <KArchive>
#include <KIO/Job>

#include <ktorrent_export.h>

namespace bt
{
class ExtractFileThread;

/*!
    Job which extracts a single file out of an archive
*/
class KTORRENT_EXPORT ExtractFileJob : public KIO::Job
{
public:
    ExtractFileJob(KArchive *archive, const QString &path, const QString &dest);
    ~ExtractFileJob() override;

    void start() override;
    virtual void kill(bool quietly = true);

private:
    void extractThreadDone();

private:
    KArchive *archive;
    QString path;
    QString dest;
    ExtractFileThread *extract_thread;
};

}

#endif // BT_EXTRACTFILEJOB_H
