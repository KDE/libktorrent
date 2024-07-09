/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dummytorrentcreator.h"
#include <QFile>
#include <torrent/torrentcreator.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

DummyTorrentCreator::DummyTorrentCreator()
    : chunk_size(256)
{
    trackers.append(QStringLiteral("http://localhost:5000/announce"));
    tmpdir.setAutoRemove(true);
}

DummyTorrentCreator::~DummyTorrentCreator()
{
}

bool DummyTorrentCreator::createMultiFileTorrent(const QMap<QString, bt::Uint64> &files, const QString &name)
{
    if (!tmpdir.isValid())
        return false;

    try {
        dpath = tmpdir.path() + QLatin1String("data") + bt::DirSeparator() + name + bt::DirSeparator();
        bt::MakePath(dpath);

        QMap<QString, bt::Uint64>::const_iterator i = files.begin();
        while (i != files.end()) {
            MakeFilePath(dpath + i.key());
            if (!createRandomFile(dpath + i.key(), i.value()))
                return false;
            ++i;
        }

        bt::TorrentCreator creator(dpath, trackers, QList<QUrl>(), chunk_size, name, "", false, false);
        // Start the hashing thread and wait until it is done
        creator.start();
        creator.wait();
        creator.saveTorrent(torrentPath());
        return true;
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_NOTICE) << "Error creating torrent: " << err.toString() << endl;
        return false;
    }
}

bool DummyTorrentCreator::createSingleFileTorrent(bt::Uint64 size, const QString &filename)
{
    if (!tmpdir.isValid())
        return false;

    try {
        bt::MakePath(tmpdir.path() + QLatin1String("data") + bt::DirSeparator());
        dpath = tmpdir.path() + QLatin1String("data") + bt::DirSeparator() + filename;
        if (!createRandomFile(dpath, size))
            return false;

        bt::TorrentCreator creator(dpath, trackers, QList<QUrl>(), chunk_size, filename, "", false, false);
        // Start the hashing thread and wait until it is done
        creator.start();
        creator.wait();
        creator.saveTorrent(torrentPath());
        return true;
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_NOTICE) << "Error creating torrent: " << err.toString() << endl;
        return false;
    }
}

bool DummyTorrentCreator::createRandomFile(const QString &path, bt::Uint64 size)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Out(SYS_GEN | LOG_NOTICE) << "Error opening " << path << ": " << file.errorString() << endl;
        return false;
    }

    bt::Uint64 written = 0;
    while (written < size) {
        char tmp[4096];
        for (int i = 0; i < 4096; i++)
            tmp[i] = rand() % 0xFF;

        bt::Uint64 to_write = 4096;
        if (to_write + written > size)
            to_write = size - written;
        written += file.write(tmp, to_write);
    }

    return true;
}

QString DummyTorrentCreator::dataPath() const
{
    return dpath;
}

QString DummyTorrentCreator::torrentPath() const
{
    return tmpdir.path() + QLatin1String("torrent");
}
