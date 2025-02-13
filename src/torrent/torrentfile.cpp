/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2005 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "torrentfile.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <cmath>
#include <diskio/chunkmanager.h>
#include <util/bitset.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
TorrentFile TorrentFile::null(nullptr);

TorrentFile::TorrentFile(Torrent *tor)
    : TorrentFileInterface(0, QString(), 0)
    , tor(tor)
    , missing(false)
{
}

TorrentFile::TorrentFile(Torrent *tor, Uint32 index, const QString &path, Uint64 off, Uint64 size, Uint64 chunk_size)
    : TorrentFileInterface(index, path, size)
    , tor(tor)
    , cache_offset(off)
    , missing(false)
{
    first_chunk = off / chunk_size;
    first_chunk_off = off % chunk_size;
    if (size > 0)
        last_chunk = (off + size - 1) / chunk_size;
    else
        last_chunk = first_chunk;
    last_chunk_size = (off + size) - last_chunk * chunk_size;
    priority = old_priority = NORMAL_PRIORITY;
}

TorrentFile::TorrentFile(const TorrentFile &tf)
    : TorrentFileInterface(tf.getIndex(), QString(), 0)
{
    setUnencodedPath(tf.unencoded_path);
    index = tf.getIndex();
    path = tf.getPath();
    size = tf.getSize();
    cache_offset = tf.getCacheOffset();
    first_chunk = tf.getFirstChunk();
    first_chunk_off = tf.getFirstChunkOffset();
    last_chunk = tf.getLastChunk();
    last_chunk_size = tf.getLastChunkSize();
    old_priority = priority = tf.getPriority();
    missing = tf.isMissing();
    tor = tf.tor;
    filetype = UNKNOWN;
}

TorrentFile::~TorrentFile()
{
}

void TorrentFile::setDoNotDownload(bool dnd)
{
    if (priority != EXCLUDED && dnd) {
        if (emit_status_changed)
            old_priority = priority;

        priority = EXCLUDED;

        if (emit_status_changed)
            tor->downloadPriorityChanged(this, priority, old_priority);
    }
    if (priority == EXCLUDED && (!dnd)) {
        if (emit_status_changed)
            old_priority = priority;

        priority = NORMAL_PRIORITY;

        if (emit_status_changed)
            tor->downloadPriorityChanged(this, priority, old_priority);
    }
}

void TorrentFile::emitDownloadStatusChanged()
{
    // only emit when old_priority is not equal to the new priority
    if (priority != old_priority)
        tor->downloadPriorityChanged(this, priority, old_priority);
}

bool TorrentFile::isMultimedia() const
{
    if (filetype == UNKNOWN) {
        QMimeType ptr = QMimeDatabase().mimeTypeForFile(getPath());
        if (!ptr.isValid()) {
            filetype = NORMAL;
            return false;
        }

        QString name = ptr.name();
        if (name.startsWith(QLatin1String("audio")) || name == QLatin1String("application/ogg"))
            filetype = AUDIO;
        else if (name.startsWith(QLatin1String("video")))
            filetype = VIDEO;
        else
            filetype = NORMAL;
    }
    return filetype == AUDIO || filetype == VIDEO;
}

void TorrentFile::setPriority(Priority newpriority)
{
    if (priority != newpriority) {
        if (priority == EXCLUDED) {
            setDoNotDownload(false);
        }

        if (newpriority == EXCLUDED) {
            setDoNotDownload(true);
            tor->filePercentageChanged(this, getDownloadPercentage());
        } else {
            old_priority = priority;
            priority = newpriority;
            tor->downloadPriorityChanged(this, newpriority, old_priority);

            // make sure percentages are updated properly
            if (old_priority == ONLY_SEED_PRIORITY || newpriority == ONLY_SEED_PRIORITY || old_priority == EXCLUDED) {
                tor->filePercentageChanged(this, getDownloadPercentage());
            }
        }
    }
}

TorrentFile &TorrentFile::operator=(const TorrentFile &tf)
{
    index = tf.getIndex();
    path = tf.getPath();
    size = tf.getSize();
    cache_offset = tf.getCacheOffset();
    first_chunk = tf.getFirstChunk();
    first_chunk_off = tf.getFirstChunkOffset();
    last_chunk = tf.getLastChunk();
    last_chunk_size = tf.getLastChunkSize();
    priority = tf.getPriority();
    missing = tf.isMissing();
    tor = tf.tor;
    return *this;
}

Uint64 TorrentFile::fileOffset(Uint32 cindex, Uint64 chunk_size) const
{
    Uint64 off = 0;
    if (getFirstChunkOffset() == 0) {
        off = (cindex - getFirstChunk()) * chunk_size;
    } else {
        if (cindex - this->getFirstChunk() > 0)
            off = (cindex - this->getFirstChunk() - 1) * chunk_size;
        if (cindex > 0)
            off += (chunk_size - this->getFirstChunkOffset());
    }
    return off;
}

void TorrentFile::updateNumDownloadedChunks(ChunkManager &cman)
{
    const BitSet &bs = cman.getBitSet();
    Uint32 old_chunk_count = num_chunks_downloaded;
    num_chunks_downloaded = 0;

    Uint32 preview_range = cman.previewChunkRangeSize(*this);
    bool prev = preview;
    preview = true;
    for (Uint32 i = first_chunk; i <= last_chunk; i++) {
        if (bs.get(i)) {
            num_chunks_downloaded++;
        } else if (preview_range > 0 && i >= first_chunk && i < first_chunk + preview_range) {
            preview = false;
        }
    }
    preview = isMultimedia() && preview;

    if (num_chunks_downloaded != old_chunk_count)
        tor->filePercentageChanged(this, getDownloadPercentage());

    if (prev != preview)
        tor->filePreviewChanged(this, preview);
}

}

#include "moc_torrentfile.cpp"
