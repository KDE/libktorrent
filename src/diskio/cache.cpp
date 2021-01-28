/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include "cache.h"
#include "cachefile.h"
#include "chunk.h"
#include "piecedata.h"
#include <KLocalizedString>
#include <peer/connectionlimit.h>
#include <peer/peermanager.h>
#include <torrent/job.h>
#include <torrent/torrent.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
bool Cache::preallocate_files = true;
bool Cache::preallocate_fully = false;

Cache::Cache(Torrent &tor, const QString &tmpdir, const QString &datadir)
    : tor(tor)
    , tmpdir(tmpdir)
    , datadir(datadir)
    , mmap_failures(0)
{
    if (!datadir.endsWith(bt::DirSeparator()))
        this->datadir += bt::DirSeparator();

    if (!tmpdir.endsWith(bt::DirSeparator()))
        this->tmpdir += bt::DirSeparator();

    preexisting_files = false;
}

Cache::~Cache()
{
    cleanupPieceCache();
}

void Cache::cleanupPieceCache()
{
    PieceCache::iterator i = piece_cache.begin();
    while (i != piece_cache.end()) {
        i.value()->unload();
        ++i;
    }
    piece_cache.clear();
}

void Cache::changeTmpDir(const QString &ndir)
{
    tmpdir = ndir;
}

bool Cache::mappedModeAllowed()
{
#ifndef Q_WS_WIN
    return MaxOpenFiles() - bt::PeerManager::connectionLimits().totalConnections() > 100;
#else
    return true; // there isn't a file handle limit on windows
#endif
}

Job *Cache::moveDataFiles(const QMap<TorrentFileInterface *, QString> &files)
{
    Q_UNUSED(files);
    return 0;
}

void Cache::moveDataFilesFinished(const QMap<TorrentFileInterface *, QString> &files, Job *job)
{
    Q_UNUSED(files);
    if (job->error())
        return;

    QSet<QString> mps;
    if (getMountPoints(mps))
        saveMountPoints(mps);
}

PieceData::Ptr Cache::findPiece(Chunk *c, Uint32 off, Uint32 len, bool read_only)
{
    PieceCache::const_iterator i = piece_cache.constFind(c);
    while (i != piece_cache.constEnd() && i.key() == c) {
        PieceData::Ptr cp = i.value();
        if (cp->offset() == off && cp->length() == len && !(!cp->writeable() && !read_only))
            return PieceData::Ptr(cp);
        ++i;
    }

    return PieceData::Ptr();
}

void Cache::insertPiece(Chunk *c, PieceData::Ptr p)
{
    piece_cache.insert(c, p);
}

void Cache::clearPieces(Chunk *c)
{
    PieceCache::iterator i = piece_cache.find(c);
    while (i != piece_cache.end() && i.key() == c) {
        i = piece_cache.erase(i);
    }
}

void Cache::clearPieceCache()
{
    PieceCache::iterator i = piece_cache.begin();
    while (i != piece_cache.end()) {
        if (!i.value()->inUse())
            i = piece_cache.erase(i);
        else
            ++i;
    }
}

void Cache::checkMemoryUsage()
{
    Uint64 mem = 0;
    Uint64 freed = 0;
    PieceCache::iterator i = piece_cache.begin();
    while (i != piece_cache.end()) {
        if (!i.value()->inUse()) {
            freed += i.value()->length();
            i = piece_cache.erase(i);
        } else {
            mem += i.value()->length();
            ++i;
        }
    }

    if (mem || freed)
        Out(SYS_DIO | LOG_DEBUG) << "Piece cache: memory in use " << BytesToString(mem) << ", memory freed " << BytesToString(freed) << endl;
}

void Cache::saveMountPoints(const QSet<QString> &mp)
{
    mount_points = mp;

    QString mp_file = tmpdir + "mount_points";
    QFile fptr(mp_file);
    if (!fptr.open(QIODevice::WriteOnly))
        throw Error(i18n("Failed to create %1: %2", mp_file, fptr.errorString()));

    QTextStream out(&fptr);
    for (const QString &mount_point : qAsConst(mount_points)) {
        out << mount_point << Qt::endl;
    }
}

void Cache::loadMountPoints()
{
    QString mp_file = tmpdir + "mount_points";
    QFile fptr(mp_file);
    if (!fptr.open(QIODevice::ReadOnly)) {
        Out(SYS_DIO | LOG_NOTICE) << "Failed to load " << mp_file << ": " << fptr.errorString() << endl;

        QSet<QString> mps;
        if (getMountPoints(mps)) {
            saveMountPoints(mps);
        }
    } else {
        mount_points.clear();
        QTextStream in(&fptr);
        while (!in.atEnd()) {
            mount_points.insert(in.readLine());
        }
    }
}

bool Cache::isStorageMounted(QStringList &missing)
{
    if (mount_points.isEmpty())
        return true;

    missing.clear();
    for (const QString &mount_point : qAsConst(mount_points)) {
        if (!IsMounted(mount_point))
            missing.append(mount_point);
    }

    return missing.empty();
}
}
