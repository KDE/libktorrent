/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "cache.h"
#include "cachefile.h"
#include "chunk.h"
#include "piecedata.h"
#include <KLocalizedString>
#include <algorithm>
#include <peer/connectionlimit.h>
#include <peer/peermanager.h>
#include <torrent/job.h>
#include <torrent/torrent.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>
#include <utility>

using namespace Qt::Literals::StringLiterals;

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
    for (const auto &piece_info_list : std::as_const(piece_cache)) {
        for (const auto &info : piece_info_list) {
            info.piece_data->unload();
        }
    }
    piece_cache.clear();
}

void Cache::changeTmpDir(const QString &ndir)
{
    tmpdir = ndir;
}

bool Cache::mappedModeAllowed()
{
#ifndef Q_OS_WIN
    return MaxOpenFiles() - bt::PeerManager::connectionLimits().totalConnections() > 100;
#else
    return true; // there isn't a file handle limit on windows
#endif
}

Job *Cache::moveDataFiles(const QMap<TorrentFileInterface *, QString> &files)
{
    Q_UNUSED(files);
    return nullptr;
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
    if (i != piece_cache.constEnd()) {
        Uint64 key = PieceDataInfo::makeKey(off, len);
        const PieceDataInfoList &info_list = i.value();
        auto j = std::lower_bound(info_list.begin(), info_list.end(), key, PieceDataInfo::OrderByKey());
        while (j != info_list.end() && j->key == key) {
            if (read_only || j->piece_data->writeable())
                return j->piece_data;
            ++j;
        }
    }

    return PieceData::Ptr();
}

void Cache::insertPiece(Chunk *c, PieceData::Ptr p)
{
    PieceDataInfoList &info_list = piece_cache[c];
    Uint64 key = PieceDataInfo::makeKey(p->offset(), p->length());
    auto i = std::upper_bound(info_list.begin(), info_list.end(), key, PieceDataInfo::OrderByKey());
    info_list.insert(i, PieceDataInfo(std::move(p), key));
}

void Cache::clearPieces(Chunk *c)
{
    piece_cache.remove(c);
}

void Cache::clearPieceCache()
{
    PieceCache::iterator i = piece_cache.begin();
    while (i != piece_cache.end()) {
        PieceDataInfoList &info_list = i.value();
        auto j = std::remove_if(info_list.begin(), info_list.end(), [](const PieceDataInfo &info) {
            return !info.piece_data->inUse();
        });
        info_list.erase(j, info_list.end());

        if (info_list.isEmpty())
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
        PieceDataInfoList &info_list = i.value();
        auto j = std::remove_if(info_list.begin(), info_list.end(), [&](const PieceDataInfo &info) {
            Uint32 len = info.piece_data->length();
            if (!info.piece_data->inUse()) {
                freed += len;
                return true;
            } else {
                mem += len;
                return false;
            }
        });
        info_list.erase(j, info_list.end());

        if (info_list.isEmpty())
            i = piece_cache.erase(i);
        else
            ++i;
    }

    if (mem || freed)
        Out(SYS_DIO | LOG_DEBUG) << "Piece cache: memory in use " << BytesToString(mem) << ", memory freed " << BytesToString(freed) << endl;
}

void Cache::saveMountPoints(const QSet<QString> &mp)
{
    mount_points = mp;

    QString mp_file = tmpdir + u"mount_points"_s;
    QFile fptr(mp_file);
    if (!fptr.open(QIODevice::WriteOnly))
        throw Error(i18n("Failed to create %1: %2", mp_file, fptr.errorString()));

    QTextStream out(&fptr);
    for (const QString &mount_point : std::as_const(mount_points)) {
        out << mount_point << Qt::endl;
    }
}

void Cache::loadMountPoints()
{
    QString mp_file = tmpdir + u"mount_points"_s;
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
    for (const QString &mount_point : std::as_const(mount_points)) {
        if (!IsMounted(mount_point))
            missing.append(mount_point);
    }

    return missing.empty();
}
}
