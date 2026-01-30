/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTMULTIFILECACHE_H
#define BTMULTIFILECACHE_H

#include "cache.h"
#include "cachefile.h"
#include "dndfile.h"
#include <QMap>
#include <memory>

namespace bt
{
/*!
 * \author Joris Guisson
 * \brief Cache for multi file torrents.
 *
 * This class manages a multi file torrent cache. Everything gets stored in the
 * correct files immediately.
 */
class KTORRENT_EXPORT MultiFileCache : public Cache
{
public:
    MultiFileCache(Torrent &tor, const QString &tmpdir, const QString &datadir, bool custom_output_name);
    ~MultiFileCache() override;

    Q_DISABLE_COPY_MOVE(MultiFileCache);

    void changeTmpDir(const QString &ndir) override;
    void create() override;
    PieceData::Ptr loadPiece(Chunk *c, Uint32 off, Uint32 length) override;
    PieceData::Ptr preparePiece(Chunk *c, Uint32 off, Uint32 length) override;
    void savePiece(PieceData::Ptr piece) override;
    void close() override;
    void open() override;
    Job *moveDataFiles(const QString &ndir) override;
    void moveDataFilesFinished(Job *job) override;
    Job *moveDataFiles(const QMap<TorrentFileInterface *, QString> &files) override;
    void moveDataFilesFinished(const QMap<TorrentFileInterface *, QString> &files, Job *job) override;
    QString getOutputPath() const override;
    void changeOutputPath(const QString &outputpath) override;
    void preparePreallocation(PreallocationThread *prealloc) override;
    bool hasMissingFiles(QStringList &sl) override;
    Job *deleteDataFiles() override;
    Uint64 diskUsage() override;
    void loadFileMap() override;
    void saveFileMap() override;
    bool getMountPoints(QSet<QString> &mps) override;

private:
    void touch(TorrentFile &tf);
    void downloadStatusChanged(TorrentFile *, bool) override;
    void saveFirstAndLastChunk(TorrentFile *tf, const QString &src_file, const QString &dst_file);
    void recreateFile(TorrentFile *tf, const QString &dnd_file, const QString &output_file);
    PieceData::Ptr createPiece(Chunk *c, Uint32 off, Uint32 length, bool read_only);
    void calculateOffsetAndLength(Uint32 piece_off, Uint32 piece_len, Uint64 file_off, Uint32 chunk_off, Uint32 chunk_len, Uint64 &off, Uint32 &len);

private:
    QString cache_dir, output_dir;
    QMap<Uint32, CacheFile::Ptr> files;
    std::map<Uint32, std::unique_ptr<DNDFile>> dnd_files;
    QString new_output_dir;
    bool opened;
};

}

#endif
