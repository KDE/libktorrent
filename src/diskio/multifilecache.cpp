/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "multifilecache.h"

#include <cerrno>

#include <KLocalizedString>

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <QTextStream>

#include <util/error.h>
#include <util/file.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>
#ifdef Q_OS_WIN
#include <util/win32.h>
#endif
#include "cache.h"
#include "cachefile.h"
#include "chunk.h"
#include "deletedatafilesjob.h"
#include "dndfile.h"
#include "movedatafilesjob.h"
#include "piecedata.h"
#include "preallocationthread.h"
#include <torrent/torrent.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
static Uint64 FileOffset(Chunk *c, const TorrentFile &f, Uint64 chunk_size);
static Uint64 FileOffset(Uint32 cindex, const TorrentFile &f, Uint64 chunk_size);
static void DeleteEmptyDirs(const QString &output_dir, const QString &fpath);

MultiFileCache::MultiFileCache(Torrent &tor, const QString &tmpdir, const QString &datadir, bool custom_output_name)
    : Cache(tor, tmpdir, datadir)
    , opened(false)
{
    cache_dir = tmpdir + "cache"_L1 + bt::DirSeparator();

    if (!custom_output_name)
        output_dir = this->datadir + tor.getNameSuggestion() + bt::DirSeparator();
    else
        output_dir = this->datadir;
}

MultiFileCache::~MultiFileCache()
{
    cleanupPieceCache();
}

void MultiFileCache::loadFileMap()
{
    QString file_map = tmpdir + "file_map"_L1;
    if (!bt::Exists(file_map)) {
        // file map doesn't exist, so just set the path on disk if it has not happened yet
        Uint32 num = tor.getNumFiles();
        for (Uint32 i = 0; i < num; i++) {
            TorrentFile &tf = tor.getFile(i);
            if (tf.getPathOnDisk().isEmpty())
                tf.setPathOnDisk(output_dir + tf.getUserModifiedPath());
        }
        saveFileMap();
    } else {
        QFile fptr(tmpdir + "file_map"_L1);
        if (!fptr.open(QIODevice::ReadOnly))
            throw Error(i18n("Failed to open %1: %2", file_map, fptr.errorString()));

        Uint32 idx = 0;
        while (!fptr.atEnd() && idx < tor.getNumFiles()) {
            QString path = QString::fromLocal8Bit(fptr.readLine().trimmed());
            tor.getFile(idx).setPathOnDisk(path);
            idx++;
        }

        // now the user modified paths must come
        idx = 0;
        while (!fptr.atEnd() && idx < tor.getNumFiles()) {
            QString path = QString::fromLocal8Bit(fptr.readLine().trimmed());
            if (!path.isEmpty())
                tor.getFile(idx).setUserModifiedPath(path);
            idx++;
        }
    }
}

void MultiFileCache::saveFileMap()
{
    QString file_map = tmpdir + "file_map"_L1;
    QFile fptr(file_map);
    if (!fptr.open(QIODevice::WriteOnly))
        throw Error(i18n("Failed to create %1: %2", file_map, fptr.errorString()));

    QTextStream out(&fptr);
    // file map doesn't exist, so create it based upon the output_dir
    Uint32 num = tor.getNumFiles();
    for (Uint32 i = 0; i < num; i++) {
        TorrentFile &tf = tor.getFile(i);
        out << tf.getPathOnDisk() << Qt::endl;
    }

    // After the actual paths on disk, save the user modified path names
    for (Uint32 i = 0; i < num; i++) {
        TorrentFile &tf = tor.getFile(i);
        out << tf.getUserModifiedPath() << Qt::endl;
    }
}

QString MultiFileCache::getOutputPath() const
{
    return output_dir;
}

void MultiFileCache::close()
{
    clearPieceCache();
    if (piece_cache.isEmpty())
        files.clear();
    opened = false;
}

void MultiFileCache::open()
{
    // Check if the cache is not yet open
    if (opened)
        return;

    QString dnd_dir = tmpdir + "dnd"_L1 + bt::DirSeparator();
    // open all files
    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        if (!tf.doNotDownload()) {
            if (files.contains(i))
                files.remove(i);

            CacheFile::Ptr fd(new CacheFile());
            fd->open(tf.getPathOnDisk(), tf.getSize());
            files.insert(i, fd);
        } else {
            if (dnd_files.contains(i))
                dnd_files.remove(i);

            QString dnd_path = u"file%1.dnd"_s.arg(tf.getIndex());
            QString dnd_file = dnd_dir + dnd_path;
            if (bt::Exists(dnd_dir + tf.getUserModifiedPath() + ".dnd"_L1)) {
                // old style dnd dir, move the file so that we can keep working
                // with the old file
                bt::Move(dnd_dir + tf.getUserModifiedPath() + ".dnd"_L1, dnd_file, true, true);
            }
            DNDFile::Ptr dfd(new DNDFile(dnd_file, &tf, tor.getChunkSize()));
            dfd->checkIntegrity();
            dnd_files.insert(i, dfd);
        }
    }

    opened = true;
}

void MultiFileCache::changeTmpDir(const QString &ndir)
{
    Cache::changeTmpDir(ndir);
    QString dnd_dir = tmpdir + "dnd"_L1 + bt::DirSeparator();

    // change paths for individual files, it should not
    // be a problem to move these files when they are open
    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        if (tf.doNotDownload()) {
            DNDFile::Ptr dfd = dnd_files[i];
            if (dfd) {
                QString dnd_path = u"file%1.dnd"_s.arg(tf.getIndex());
                dfd->changePath(dnd_dir + dnd_path);
            }
        }
    }
}

void MultiFileCache::changeOutputPath(const QString &outputpath)
{
    output_dir = outputpath;
    if (!output_dir.endsWith(bt::DirSeparator()))
        output_dir += bt::DirSeparator();

    datadir = output_dir;

    Uint32 num = tor.getNumFiles();
    for (Uint32 i = 0; i < num; i++) {
        TorrentFile &tf = tor.getFile(i);
        tf.setPathOnDisk(output_dir + tf.getUserModifiedPath());
        CacheFile::Ptr cf = files[tf.getIndex()];
        if (cf)
            cf->changePath(tf.getPathOnDisk());
    }
    saveFileMap();
}

Job *MultiFileCache::moveDataFiles(const QString &ndir)
{
    if (!bt::Exists(ndir))
        bt::MakeDir(ndir);

    QString nd = ndir;
    if (!nd.endsWith(bt::DirSeparator()))
        nd += bt::DirSeparator();

    new_output_dir = nd;

    MoveDataFilesJob *job = new MoveDataFilesJob();
    int nmoves = 0;

    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        if (tf.doNotDownload())
            continue;

        // check if every directory along the path exists, and if it doesn't
        // create it
        MakeFilePath(nd + tf.getUserModifiedPath());

        if (QFileInfo(tf.getPathOnDisk()).canonicalPath() != QFileInfo(nd + tf.getUserModifiedPath()).canonicalPath()) {
            job->addMove(tf.getPathOnDisk(), nd + tf.getUserModifiedPath());
            nmoves++;
        }
    }

    if (nmoves == 0) {
        delete job;
        return nullptr;
    } else {
        return job;
    }
}

void MultiFileCache::moveDataFilesFinished(Job *job)
{
    if (job->error())
        return;

    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        tf.setPathOnDisk(new_output_dir + tf.getUserModifiedPath());
        CacheFile::Ptr cf = files[tf.getIndex()];
        if (cf)
            cf->changePath(tf.getPathOnDisk());
        // check for empty directories and delete them
        DeleteEmptyDirs(output_dir, tf.getUserModifiedPath());
    }
}

Job *MultiFileCache::moveDataFiles(const QMap<TorrentFileInterface *, QString> &files)
{
    if (files.count() == 0)
        return nullptr;

    MoveDataFilesJob *job = new MoveDataFilesJob(files);
    return job;
}

void MultiFileCache::moveDataFilesFinished(const QMap<TorrentFileInterface *, QString> &fmap, Job *job)
{
    if (job->error())
        return;

    QMap<TorrentFileInterface *, QString>::const_iterator i = fmap.begin();
    while (i != fmap.end()) {
        TorrentFileInterface *tf = i.key();
        QString path = tf->getPathOnDisk();
        QString dest = i.value();
        if (QFileInfo(dest).isDir()) {
            QString path = tf->getUserModifiedPath();
            if (!dest.endsWith(bt::DirSeparator()))
                dest += bt::DirSeparator();

            int last = path.lastIndexOf(bt::DirSeparator());
            tf->setPathOnDisk(dest + path.mid(last + 1));
        } else
            tf->setPathOnDisk(i.value());

        CacheFile::Ptr cf = files[tf->getIndex()];
        if (cf)
            cf->changePath(tf->getPathOnDisk());
        ++i;
    }

    Cache::moveDataFilesFinished(fmap, job);
    saveFileMap();
}

void MultiFileCache::create()
{
    if (!bt::Exists(output_dir))
        MakeDir(output_dir);
    if (!bt::Exists(tmpdir + "dnd"_L1))
        bt::MakeDir(tmpdir + "dnd"_L1);

    QSet<QString> mount_points;
    QSet<QString> shortened_names;
    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
#ifndef Q_OS_WIN
        // check if the filename is to long
        if (FileNameToLong(tf.getPathOnDisk())) {
            QString s = ShortenFileName(tf.getPathOnDisk());
            Out(SYS_DIO | LOG_DEBUG) << "Path to long " << tf.getPathOnDisk() << endl;
            // make sure there are no dupes
            int cnt = 1;
            while (shortened_names.contains(s)) {
                s = ShortenFileName(tf.getPathOnDisk(), cnt++);
            }
            Out(SYS_DIO | LOG_DEBUG) << "Shortened to " << s << endl;

            tf.setPathOnDisk(s);
            shortened_names.insert(s);
        }
#endif
        touch(tf);
        if (!tf.doNotDownload())
            mount_points.insert(tf.getMountPoint());
    }

    saveMountPoints(mount_points);
    saveFileMap();
}

void MultiFileCache::touch(TorrentFile &tf)
{
    QString fpath = tf.getUserModifiedPath();
    bool dnd = tf.doNotDownload();
    // first split fpath by / separator
    QStringList sl = fpath.split(bt::DirSeparator());

    // then make the file
    if (!dnd) {
        MakeFilePath(tf.getPathOnDisk());
        if (!bt::Exists(tf.getPathOnDisk())) {
            bt::Touch(tf.getPathOnDisk());
        } else {
            preexisting_files = true;
            tf.setPreExisting(true); // mark the file as preexisting
        }
    }
}

PieceData::Ptr MultiFileCache::createPiece(Chunk *c, Uint32 off, Uint32 length, bool read_only)
{
    open();

    Torrent::FileIndexList tflist;
    tor.calcChunkPos(c->getIndex(), tflist, 2); // we don't need all the files, just to know whether there is only one or multiple files

    // one file so try to map it
    if (tflist.count() == 1) {
        const TorrentFile &f = tor.getFile(tflist.first());
        CacheFile::Ptr fd = files[tflist.first()];
        if (!fd)
            return PieceData::Ptr();

        if (Cache::mappedModeAllowed() && mmap_failures < 3) {
            Uint64 offset = FileOffset(c, f, tor.getChunkSize()) + off;
            PieceData::Ptr piece(new PieceData(c, off, length, nullptr, fd, read_only));
            Uint8 *buf = (Uint8 *)fd->map(piece.data(), offset, length, read_only ? CacheFile::READ : CacheFile::RW);
            if (buf) {
                piece->setData(buf);
                insertPiece(c, piece);
                return piece;
            } else {
                mmap_failures++;
            }
        }
    }

    // mmap failed or there are multiple files, so just do buffered
    Uint8 *buf = new Uint8[length];
    PieceData::Ptr piece(new PieceData(c, off, length, buf, CacheFile::Ptr(), read_only));
    insertPiece(c, piece);
    return piece;
}

PieceData::Ptr MultiFileCache::preparePiece(Chunk *c, Uint32 off, Uint32 length)
{
    PieceData::Ptr piece = findPiece(c, off, length, false);
    if (piece)
        return piece;

    return createPiece(c, off, length, false);
}

void MultiFileCache::calculateOffsetAndLength(Uint32 piece_off, Uint32 piece_len, Uint64 file_off, Uint32 chunk_off, Uint32 chunk_len, Uint64 &off, Uint32 &len)
{
    // if the piece offset lies in the range of the current chunk, we need to read data
    if (piece_off >= chunk_off && piece_off + piece_len <= chunk_off + chunk_len) {
        // The piece lies entirely in the current file
        off = file_off + (piece_off - chunk_off);
        len = piece_len;
    } else if (piece_off >= chunk_off && piece_off < chunk_off + chunk_len) {
        // The start of the piece lies partially in the current file
        off = file_off + (piece_off - chunk_off);
        len = chunk_len - (piece_off - chunk_off);
    } else if (piece_off < chunk_off && piece_off + piece_len > chunk_off && piece_off + piece_len <= chunk_off + chunk_len) {
        // The end of the piece lies in the file
        off = file_off;
        len = piece_len - (chunk_off - piece_off);
    } else if (chunk_off >= piece_off && chunk_off + chunk_len < piece_off + piece_len) {
        // the current file lies entirely in the piece
        off = file_off;
        len = chunk_len;
    }
}

PieceData::Ptr MultiFileCache::loadPiece(Chunk *c, Uint32 off, Uint32 length)
{
    open();

    PieceData::Ptr piece = findPiece(c, off, length, true);
    if (piece)
        return piece;

    // create piece and return it if it is mapped or the create failed
    piece = createPiece(c, off, length, true);
    if (!piece || piece->mapped())
        return piece;

    // Now we need to load it
    Torrent::FileIndexList tflist;
    tor.calcChunkPos(c->getIndex(), tflist);

    // The chunk lies in one file, so it is easy
    if (tflist.count() == 1) {
        const TorrentFile &f = tor.getFile(tflist[0]);
        CacheFile::Ptr fd = files[tflist[0]];
        Uint64 piece_off = FileOffset(c, f, tor.getChunkSize()) + off;

        fd->read(piece->data(), length, piece_off);
        return piece;
    }

    // multiple files
    Uint8 *data = piece->data();
    Uint32 chunk_off = 0; // number of bytes passed of the chunk
    Uint32 piece_off = 0; // how many bytes read to the piece
    for (int i = 0; i < tflist.count(); i++) {
        const TorrentFile &f = tor.getFile(tflist[i]);
        CacheFile::Ptr fd = files[tflist[i]];
        DNDFile::Ptr dfd = dnd_files[tflist[i]];

        // first calculate offset into file
        // only the first file can have an offset
        // the following files will start at the beginning
        Uint64 file_off = 0;
        if (i == 0)
            file_off = FileOffset(c, f, tor.getChunkSize());

        Uint32 cdata = 0;
        // then the amount of data of the chunk which is located in this file
        if (tflist.count() == 1)
            cdata = c->getSize();
        else if (i == 0)
            cdata = f.getLastChunkSize();
        else if (i == tflist.count() - 1)
            cdata = c->getSize() - chunk_off;
        else
            cdata = f.getSize();

        // if the piece does not lie in this part of the chunk, move on
        if (off + length <= chunk_off || off >= chunk_off + cdata) {
            chunk_off += cdata;
            continue;
        }

        Uint64 read_offset = 0; // The read offset in the file
        Uint32 read_length = 0; // how many bytes to read
        calculateOffsetAndLength(off, length, file_off, chunk_off, cdata, read_offset, read_length);

        Uint8 *ptr = data + piece_off; // location to write to
        piece_off += read_length;

        if (fd) {
            fd->read(ptr, read_length, read_offset);
        } else if (dfd) {
            Uint32 ret = 0;
            if (i == 0)
                ret = dfd->readLastChunk(ptr, read_offset - file_off, read_length);
            else
                ret = dfd->readFirstChunk(ptr, read_offset, read_length);

            if (ret > 0 && ret != read_length)
                Out(SYS_DIO | LOG_DEBUG) << "Warning : MultiFileCache::loadPiece ret != to_read" << endl;
        }

        chunk_off += cdata;
    }

    return piece;
}

void MultiFileCache::savePiece(PieceData::Ptr piece)
{
    open();

    // in mapped mode unload the piece if not in use
    if (piece->mapped())
        return;

    Uint8 *data = piece->data();
    if (!data) // this should not happen but just in case
        return;

    Chunk *c = piece->parentChunk();
    Torrent::FileIndexList tflist;
    tor.calcChunkPos(c->getIndex(), tflist);
    Uint32 chunk_off = 0; // number of bytes passed of the chunk
    Uint32 piece_off = 0; // how many bytes written from the piece
    Uint32 off = piece->offset();
    Uint32 length = piece->length();

    for (int i = 0; i < tflist.count(); i++) {
        const TorrentFile &f = tor.getFile(tflist[i]);
        CacheFile::Ptr fd = files[tflist[i]];
        DNDFile::Ptr dfd = dnd_files[tflist[i]];

        // first calculate offset into file
        // only the first file can have an offset
        // the following files will start at the beginning
        Uint64 file_off = 0;
        if (i == 0)
            file_off = FileOffset(c, f, tor.getChunkSize());

        Uint32 cdata = 0;
        // then the amount of data of the chunk which is located in this file
        if (tflist.count() == 1)
            cdata = c->getSize();
        else if (i == 0)
            cdata = f.getLastChunkSize();
        else if (i == tflist.count() - 1)
            cdata = c->getSize() - chunk_off;
        else
            cdata = f.getSize();

        // if the piece does not lie in this part of the chunk, move on
        if (off + length <= chunk_off || off >= chunk_off + cdata) {
            chunk_off += cdata;
            continue;
        }

        Uint64 write_offset = 0; // The write offset in the file
        Uint32 write_length = 0; // how many bytes to write
        calculateOffsetAndLength(off, length, file_off, chunk_off, cdata, write_offset, write_length);

        Uint8 *ptr = data + piece_off; // location to read from
        piece_off += write_length;

        if (fd) {
            fd->write(ptr, write_length, write_offset);
        } else if (dfd) {
            if (i == 0)
                dfd->writeLastChunk(ptr, write_offset - file_off, write_length);
            else
                dfd->writeFirstChunk(ptr, write_offset, write_length);
        }

        chunk_off += cdata;
    }
}

void MultiFileCache::downloadStatusChanged(TorrentFile *tf, bool download)
{
    bool dnd = !download;
    QString dnd_dir = tmpdir + "dnd"_L1 + bt::DirSeparator();
    QString dnd_path = u"file%1.dnd"_s.arg(tf->getIndex());
    QString dnd_file = dnd_dir + dnd_path;
    // if it is dnd and it is already in the dnd tree do nothing
    if (dnd && bt::Exists(dnd_dir + dnd_path))
        return;
    else if (dnd && bt::Exists(dnd_dir + tf->getUserModifiedPath() + ".dnd"_L1)) {
        // old style dnd dir, move the file so that we can keep working
        // with the old file
        bt::Move(dnd_dir + tf->getUserModifiedPath() + ".dnd"_L1, dnd_file, true, true);
        return;
    }

    // if it is !dnd and it is already in the output_dir tree do nothing
    if (!dnd && bt::Exists(tf->getPathOnDisk()))
        return;

    try {
        if (dnd) {
            // save first and last chunk of the file
            if (bt::Exists(tf->getPathOnDisk()))
                saveFirstAndLastChunk(tf, tf->getPathOnDisk(), dnd_file);

            // delete data file
            if (bt::Exists(tf->getPathOnDisk()))
                bt::Delete(tf->getPathOnDisk(), true);

            files.remove(tf->getIndex());
            DNDFile::Ptr dfd(new DNDFile(dnd_file, tf, tor.getChunkSize()));
            dfd->checkIntegrity();
            dnd_files.insert(tf->getIndex(), dfd);
        } else {
            // recreate the file
            recreateFile(tf, dnd_dir + dnd_path, tf->getPathOnDisk());
            bt::Delete(dnd_dir + dnd_path);
            dnd_files.remove(tf->getIndex());
            CacheFile::Ptr fd(new CacheFile());
            fd->open(tf->getPathOnDisk(), tf->getSize());
            files.insert(tf->getIndex(), fd);
        }
    } catch (bt::Error &err) {
        Out(SYS_DIO | LOG_DEBUG) << err.toString() << endl;
    }
}

void MultiFileCache::saveFirstAndLastChunk(TorrentFile *tf, const QString &src_file, const QString &dst_file)
{
    DNDFile out(dst_file, tf, tor.getChunkSize());
    File fptr;
    if (!fptr.open(src_file, u"rb"_s))
        throw Error(i18n("Cannot open file %1: %2", src_file, fptr.errorString()));

    Uint32 cs = (tf->getFirstChunk() == tor.getNumChunks() - 1) ? tor.getLastChunkSize() : tor.getChunkSize();

    Uint8 *tmp = new Uint8[tor.getChunkSize()];
    try {
        fptr.read(tmp, cs - tf->getFirstChunkOffset());
        out.writeFirstChunk(tmp, 0, cs - tf->getFirstChunkOffset());

        if (tf->getFirstChunk() != tf->getLastChunk()) {
            Uint64 off = FileOffset(tf->getLastChunk(), *tf, tor.getChunkSize());
            fptr.seek(File::BEGIN, off);
            fptr.read(tmp, tf->getLastChunkSize());
            out.writeLastChunk(tmp, 0, tf->getLastChunkSize());
        }
        delete[] tmp;
    } catch (...) {
        delete[] tmp;
        throw;
    }
}

void MultiFileCache::recreateFile(TorrentFile *tf, const QString &dnd_file, const QString &output_file)
{
    DNDFile dnd(dnd_file, tf, tor.getChunkSize());

    // make sure path exists
    MakeFilePath(output_file);
    // create the output file
    bt::Touch(output_file);

    Uint32 cs = (tf->getFirstChunk() == tor.getNumChunks() - 1) ? tor.getLastChunkSize() : tor.getChunkSize();

    File fptr;
    if (!fptr.open(output_file, u"r+b"_s))
        throw Error(i18n("Cannot open file %1: %2", output_file, fptr.errorString()));

    Uint32 ts = cs - tf->getFirstChunkOffset() > tf->getLastChunkSize() ? cs - tf->getFirstChunkOffset() : tf->getLastChunkSize();
    Uint8 *tmp = new Uint8[ts];

    try {
        Uint32 to_read = cs - tf->getFirstChunkOffset();
        if (to_read > tf->getSize()) // check for files which are smaller then a chunk
            to_read = tf->getSize();

        to_read = dnd.readFirstChunk(tmp, 0, to_read);
        if (to_read > 0)
            fptr.write(tmp, to_read);

        if (tf->getFirstChunk() != tf->getLastChunk()) {
            Uint64 off = FileOffset(tf->getLastChunk(), *tf, tor.getChunkSize());
            fptr.seek(File::BEGIN, off);
            to_read = dnd.readLastChunk(tmp, 0, tf->getLastChunkSize());
            if (to_read > 0)
                fptr.write(tmp, to_read);
        }
        delete[] tmp;

    } catch (...) {
        delete[] tmp;
        throw;
    }
}

void MultiFileCache::preparePreallocation(PreallocationThread *prealloc)
{
    QMap<Uint32, CacheFile::Ptr>::iterator i = files.begin();
    while (i != files.end()) {
        CacheFile::Ptr cf = i.value();
        if (cf)
            prealloc->add(cf);
        ++i;
    }
}

bool MultiFileCache::hasMissingFiles(QStringList &sl)
{
    bool ret = false;
    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        if (tf.doNotDownload())
            continue;

        QString p = tf.getPathOnDisk();
        if (!bt::Exists(p)) {
            ret = true;
            tf.setMissing(true);
            sl.append(p);
        } else
            tf.setMissing(false);
    }
    return ret;
}

static void DeleteEmptyDirs(const QString &output_dir, const QString &fpath)
{
    QStringList sl = fpath.split(bt::DirSeparator());
    // remove the last, which is just the filename
    sl.pop_back();

    while (sl.count() > 0) {
        QString path = output_dir;
        // reassemble the full directory path
        for (const QString &s : sl)
            path += s + bt::DirSeparator();

        QDir dir(path);
        if (dir.exists() && dir.isEmpty(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot)) {
            // no childern so delete the directory
            Out(SYS_GEN | LOG_DEBUG) << "Deleting empty directory : " << path << endl;
            bt::Delete(path, true);
            sl.pop_back(); // remove the last so we can go one higher
        } else {
            // children, so we cannot delete any more directories higher up
            return;
        }
    }

    // now the output_dir itself
    QDir dir(output_dir);
    if (dir.exists() && dir.isEmpty(QDir::AllEntries | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot)) {
        Out(SYS_GEN | LOG_DEBUG) << "Deleting empty directory : " << output_dir << endl;
        bt::Delete(output_dir, true);
    }
}

Job *MultiFileCache::deleteDataFiles()
{
    DeleteDataFilesJob *job = new DeleteDataFilesJob(output_dir);
    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        QString fpath = tf.getPathOnDisk();
        if (!tf.doNotDownload()) {
            // first delete the file
            job->addFile(fpath);
        }

        // check for subdirectories
        job->addEmptyDirectoryCheck(tf.getUserModifiedPath());
    }

    return job;
}

Uint64 MultiFileCache::diskUsage()
{
    Uint64 sum = 0;

    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        if (tf.doNotDownload())
            continue;

        try {
            CacheFile::Ptr cf = files[i];
            if (cf) {
                sum += cf->diskUsage();
            } else if (bt::Exists(tf.getPathOnDisk())) {
                // doesn't exist yet, must be before open is called
                // so create one and delete it right after
                CacheFile::Ptr cf(new CacheFile());
                cf->open(tf.getPathOnDisk(), tf.getSize());
                sum += cf->diskUsage();
            }
        } catch (bt::Error &err) { // make sure we catch any exceptions
            Out(SYS_DIO | LOG_DEBUG) << "Error: " << err.toString() << endl;
        }
    }

    return sum;
}

bool MultiFileCache::getMountPoints(QSet<QString> &mps)
{
    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        if (tf.doNotDownload())
            continue;

        QString mp = tf.getMountPoint();
        if (mp.isEmpty())
            return false;

        mps.insert(mp);
    }

    return true;
}

///////////////////////////////

Uint64 FileOffset(Chunk *c, const TorrentFile &f, Uint64 chunk_size)
{
    return FileOffset(c->getIndex(), f, chunk_size);
}

Uint64 FileOffset(Uint32 cindex, const TorrentFile &f, Uint64 chunk_size)
{
    return f.fileOffset(cindex, chunk_size);
}

}
