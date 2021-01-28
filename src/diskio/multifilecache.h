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
#ifndef BTMULTIFILECACHE_H
#define BTMULTIFILECACHE_H

#include "cache.h"
#include "cachefile.h"
#include "dndfile.h"
#include <QMap>

namespace bt
{
/**
 * @author Joris Guisson
 * @brief Cache for multi file torrents
 *
 * This class manages a multi file torrent cache. Everything gets stored in the
 * correct files immediately.
 */
class KTORRENT_EXPORT MultiFileCache : public Cache
{
public:
    MultiFileCache(Torrent &tor, const QString &tmpdir, const QString &datadir, bool custom_output_name);
    ~MultiFileCache() override;

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
    QMap<Uint32, DNDFile::Ptr> dnd_files;
    QString new_output_dir;
    bool opened;
};

}

#endif
