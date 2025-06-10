/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSINGLEFILECACHE_H
#define BTSINGLEFILECACHE_H

#include "cache.h"

namespace bt
{
class CacheFile;

/*!
 * \author Joris Guisson
 * \brief Cache for single file torrents.
 *
 * This class implements Cache for a single file torrent
 */
class KTORRENT_EXPORT SingleFileCache : public Cache
{
public:
    SingleFileCache(Torrent &tor, const QString &tmpdir, const QString &datadir);
    ~SingleFileCache() override;

    PieceData::Ptr loadPiece(Chunk *c, Uint32 off, Uint32 length) override;
    PieceData::Ptr preparePiece(Chunk *c, Uint32 off, Uint32 length) override;
    void savePiece(PieceData::Ptr piece) override;
    void create() override;
    void close() override;
    void open() override;
    void changeTmpDir(const QString &ndir) override;
    using Cache::moveDataFiles;
    Job *moveDataFiles(const QString &ndir) override;
    using Cache::moveDataFilesFinished;
    void moveDataFilesFinished(Job *job) override;
    void changeOutputPath(const QString &outputpath) override;
    QString getOutputPath() const override
    {
        return output_file;
    }
    void preparePreallocation(PreallocationThread *prealloc) override;
    bool hasMissingFiles(QStringList &sl) override;
    Job *deleteDataFiles() override;
    Uint64 diskUsage() override;
    void loadFileMap() override;
    void saveFileMap() override;
    bool getMountPoints(QSet<QString> &mps) override;

private:
    PieceData::Ptr createPiece(Chunk *c, Uint64 off, Uint32 length, bool read_only);

private:
    QString cache_file;
    QString output_file;
    QString move_data_files_dst;
    CacheFile::Ptr fd;
};

}

#endif
