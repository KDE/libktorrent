/*
    SPDX-FileCopyrightText: 2005 Joris Guisson & Maggioni Marcello <joris.guisson@gmail.com>
    marcello.maggioni@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "multidatachecker.h"
#include <diskio/dndfile.h>
#include <klocalizedstring.h>
#include <torrent/globals.h>
#include <torrent/torrent.h>
#include <torrent/torrentfile.h>
#include <util/array.h>
#include <util/error.h>
#include <util/file.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
MultiDataChecker::MultiDataChecker(bt::Uint32 from, bt::Uint32 to)
    : DataChecker(from, to)
    , buf(nullptr)
{
}

MultiDataChecker::~MultiDataChecker()
{
    delete[] buf;
}

void MultiDataChecker::check(const QString &path, const Torrent &tor, const QString &dnddir, const BitSet &current_status)
{
    Uint32 num_chunks = tor.getNumChunks();
    // initialize the bitset
    result = BitSet(num_chunks);

    if (from >= tor.getNumChunks())
        from = 0;
    if (to >= tor.getNumChunks())
        to = tor.getNumChunks() - 1;

    cache = path;
    if (!cache.endsWith(bt::DirSeparator()))
        cache += bt::DirSeparator();

    dnd_dir = dnddir;
    if (!dnddir.endsWith(bt::DirSeparator()))
        dnd_dir += bt::DirSeparator();

    Uint64 chunk_size = tor.getChunkSize();
    Uint32 cur_chunk = 0;
    buf = new Uint8[chunk_size];

    TimeStamp last_emitted = bt::Now();
    for (cur_chunk = from; cur_chunk <= to && !need_to_stop; cur_chunk++) {
        Uint32 cs = (cur_chunk == num_chunks - 1) ? tor.getLastChunkSize() : chunk_size;
        if (cs == 0)
            cs = chunk_size;
        if (!loadChunk(cur_chunk, cs, tor)) {
            // Out(SYS_GEN|LOG_DEBUG) << "Failed to load chunk " << cur_chunk << endl;
            if (current_status.get(cur_chunk))
                failed++;
            else
                not_downloaded++;
            continue;
        }

        bool ok = (SHA1Hash::generate(buf, cs) == tor.getHash(cur_chunk));
        result.set(cur_chunk, ok);
        if (ok && current_status.get(cur_chunk))
            downloaded++;
        else if (!ok && current_status.get(cur_chunk))
            failed++;
        else if (!ok && !current_status.get(cur_chunk))
            not_downloaded++;
        else if (ok && !current_status.get(cur_chunk))
            found++;

        TimeStamp now = Now();
        if (now - last_emitted > 1000 || cur_chunk == num_chunks - 1) { // Emit signals once every second
            Q_EMIT status(failed, found, downloaded, not_downloaded);
            Q_EMIT progress(cur_chunk - from, from - to + 1);
            last_emitted = now;
        }
    }

    Q_EMIT status(failed, found, downloaded, not_downloaded);
}

bool MultiDataChecker::loadChunk(Uint32 ci, Uint32 cs, const Torrent &tor)
{
    QList<Uint32> tflist;
    tor.calcChunkPos(ci, tflist);

    closePastFiles(tflist.first());

    // one file is simple
    if (tflist.count() == 1) {
        const TorrentFile &f = tor.getFile(tflist.first());
        if (!f.doNotDownload()) {
            File::Ptr fptr = open(tor, tflist.first());
            Uint64 off = f.fileOffset(ci, tor.getChunkSize());
            if (fptr->seek(File::BEGIN, off) != off)
                return false;

            return fptr->read(buf, cs) == cs;
        }
        return false;
    }

    Uint64 read = 0; // number of bytes read
    for (int i = 0; i < tflist.count(); i++) {
        const TorrentFile &f = tor.getFile(tflist[i]);

        // first calculate offset into file
        // only the first file can have an offset
        // the following files will start at the beginning
        Uint64 off = 0;
        if (i == 0)
            off = f.fileOffset(ci, tor.getChunkSize());

        Uint32 to_read = 0;
        // then the amount of data we can read from this file
        if (i == 0)
            to_read = f.getLastChunkSize();
        else if (i == tflist.count() - 1)
            to_read = cs - read;
        else
            to_read = f.getSize();

        // read part of data
        if (f.doNotDownload()) {
            QString dnd_path = QString("file%1.dnd").arg(f.getIndex());
            QString dnd_file = dnd_dir + dnd_path;
            if (!bt::Exists(dnd_file)) // could be an old style dnd dir
                dnd_file = dnd_dir + f.getUserModifiedPath() + ".dnd";

            if (bt::Exists(dnd_file)) {
                Uint32 ret = 0;
                DNDFile dfd(dnd_file, &f, tor.getChunkSize());
                if (i == 0)
                    ret = dfd.readLastChunk(buf + read, 0, to_read);
                else
                    ret = dfd.readFirstChunk(buf + read, 0, to_read);

                if (ret > 0 && ret != to_read)
                    Out(SYS_GEN | LOG_DEBUG) << "Warning : MultiDataChecker::load ret != to_read (dnd)" << endl;
            }
        } else {
            try {
                if (!bt::Exists(f.getPathOnDisk()) || bt::FileSize(f.getPathOnDisk()) < off)
                    return false;

                File::Ptr fptr = open(tor, tflist.at(i));
                if (fptr->seek(File::BEGIN, off) != off)
                    return false;

                if (fptr->read(buf + read, to_read) != to_read)
                    return false;
            } catch (bt::Error &err) {
                Out(SYS_GEN | LOG_NOTICE) << err.toString() << endl;
                return false;
            }
        }
        read += to_read;
    }
    return true;
}

File::Ptr MultiDataChecker::open(const bt::Torrent &tor, Uint32 idx)
{
    QMap<Uint32, File::Ptr>::iterator i = files.find(idx);
    if (i != files.end())
        return i.value();

    const TorrentFile &tf = tor.getFile(idx);
    File::Ptr fptr(new File());
    if (!fptr->open(tf.getPathOnDisk(), "rb")) {
        QString err = i18n("Cannot open file %1: %2", tf.getPathOnDisk(), fptr->errorString());
        Out(SYS_GEN | LOG_DEBUG) << err << endl;
        throw Error(err);
    } else {
        files.insert(idx, fptr);
        return fptr;
    }
}

void MultiDataChecker::closePastFiles(Uint32 min_idx)
{
    QMap<Uint32, File::Ptr>::iterator i = files.begin();
    while (i != files.end()) {
        if (i.key() < min_idx)
            i = files.erase(i);
        else {
            ++i;
        }
    }
}

}
