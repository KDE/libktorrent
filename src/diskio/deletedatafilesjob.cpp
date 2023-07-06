/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "deletedatafilesjob.h"
#include <QDir>
#include <kio/deletejob.h>
#include <kio/jobuidelegate.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
DeleteDataFilesJob::DeleteDataFilesJob(const QString &base)
    : Job(true, nullptr)
    , base(base)
    , directory_tree(nullptr)
{
}

DeleteDataFilesJob::~DeleteDataFilesJob()
{
    delete directory_tree;
}

void DeleteDataFilesJob::addFile(const QString &file)
{
    files.append(QUrl::fromLocalFile(file));
}

void DeleteDataFilesJob::addEmptyDirectoryCheck(const QString &fpath)
{
    if (!directory_tree)
        directory_tree = new DirTree(base);

    directory_tree->insert(fpath);
}

void DeleteDataFilesJob::start()
{
    active_job = KIO::del(files, KIO::HideProgressInfo);
    connect(active_job, &KIO::Job::result, this, &DeleteDataFilesJob::onDeleteJobDone);
}

void DeleteDataFilesJob::onDeleteJobDone(KJob *j)
{
    if (j != active_job)
        return;

    if (active_job->error())
        active_job->uiDelegate()->showErrorMessage();
    active_job = nullptr;

    if (directory_tree)
        directory_tree->doDeleteOnEmpty(base);

    setError(0);
    emitResult();
}

void DeleteDataFilesJob::kill(bool quietly)
{
    Q_UNUSED(quietly);
}

DeleteDataFilesJob::DirTree::DirTree(const QString &name)
    : name(name)
{
    subdirs.setAutoDelete(true);
}

DeleteDataFilesJob::DirTree::~DirTree()
{
}

void DeleteDataFilesJob::DirTree::insert(const QString &fpath)
{
    int i = fpath.indexOf(bt::DirSeparator());
    if (i == -1) // last part of fpath is a file, so we need to ignore that
        return;

    QString dn = fpath.left(i);
    DirTree *d = subdirs.find(dn);
    if (!d) {
        d = new DirTree(dn);
        subdirs.insert(dn, d);
    }

    d->insert(fpath.mid(i + 1));
}

void DeleteDataFilesJob::DirTree::doDeleteOnEmpty(const QString &base)
{
    bt::PtrMap<QString, DirTree>::iterator i = subdirs.begin();
    while (i != subdirs.end()) {
        i->second->doDeleteOnEmpty(base + i->first + bt::DirSeparator());
        ++i;
    }

    QDir dir(base);
    QStringList el = dir.entryList(QDir::AllEntries | QDir::System | QDir::Hidden);
    el.removeAll(".");
    el.removeAll("..");
    if (el.count() == 0) {
        // no childern so delete the directory
        Out(SYS_DIO | LOG_DEBUG) << "Deleting empty directory : " << base << endl;
        bt::Delete(base, true);
    }
}
}

#include "moc_deletedatafilesjob.cpp"
