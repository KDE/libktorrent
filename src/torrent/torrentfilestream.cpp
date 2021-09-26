/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "torrentfilestream.h"

#include <QPointer>

#include "torrentcontrol.h"
#include <diskio/chunkmanager.h>
#include <diskio/piecedata.h>
#include <download/streamingchunkselector.h>
#include <util/timer.h>

namespace bt
{
class TorrentFileStream::Private
{
public:
    Private(TorrentControl *tc, ChunkManager *cman, bool streaming_mode, TorrentFileStream *p);
    Private(TorrentControl *tc, Uint32 file_index, ChunkManager *cman, bool streaming_mode, TorrentFileStream *p);
    ~Private();

    void reset();
    void update();
    Uint32 lastChunk();
    Uint32 firstChunk();
    Uint32 firstChunkOffset();
    Uint32 lastChunkSize();
    qint64 readData(char *data, qint64 maxlen);
    qint64 readCurrentChunk(char *data, qint64 maxlen);
    bool seek(qint64 pos);

public:
    QPointer<TorrentControl> tc;
    Uint32 file_index;
    ChunkManager *cman;
    TorrentFileStream *p;
    Uint64 current_byte_offset;
    Uint64 bytes_readable;
    bool opened;

    PieceData::Ptr current_chunk_data;
    Uint32 current_chunk;
    Uint32 current_chunk_offset;
    bt::Timer timer;
    StreamingChunkSelector *csel;

    BitSet bitset;
};

TorrentFileStream::TorrentFileStream(TorrentControl *tc, ChunkManager *cman, bool streaming_mode, QObject *parent)
    : QIODevice(parent)
    , d(new Private(tc, cman, streaming_mode, this))
{
}

TorrentFileStream::TorrentFileStream(TorrentControl *tc, Uint32 file_index, ChunkManager *cman, bool streaming_mode, QObject *parent)
    : QIODevice(parent)
    , d(new Private(tc, file_index, cman, streaming_mode, this))
{
}

TorrentFileStream::~TorrentFileStream()
{
    delete d;
}

const bt::BitSet &TorrentFileStream::chunksBitSet() const
{
    return d->bitset;
}

Uint32 TorrentFileStream::currentChunk() const
{
    return d->current_chunk - d->firstChunk();
}

qint64 TorrentFileStream::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return -1;
}

qint64 TorrentFileStream::readData(char *data, qint64 maxlen)
{
    return d->readData(data, maxlen);
}

bool TorrentFileStream::open(QIODevice::OpenMode mode)
{
    if (mode != QIODevice::ReadOnly)
        return false;

    if (d->opened)
        return true;

    QIODevice::open(mode | QIODevice::Unbuffered);
    d->opened = true;
    return true;
}

void TorrentFileStream::close()
{
    d->reset();
    d->opened = false;
}

qint64 TorrentFileStream::pos() const
{
    return d->current_byte_offset;
}

qint64 TorrentFileStream::size() const
{
    if (!d->tc)
        return 0;

    if (d->tc->getStats().multi_file_torrent) {
        return d->tc->getTorrentFile(d->file_index).getSize();
    } else {
        return d->tc->getStats().total_bytes;
    }
}

bool TorrentFileStream::seek(qint64 pos)
{
    d->update();
    return d->seek(pos);
}

bool TorrentFileStream::atEnd() const
{
    return pos() >= size();
}

bool TorrentFileStream::reset()
{
    if (!d->opened)
        return false;

    d->reset();
    return true;
}

qint64 TorrentFileStream::bytesAvailable() const
{
    return d->bytes_readable;
}

QString TorrentFileStream::path() const
{
    if (!d->tc)
        return QString();

    if (d->tc->getStats().multi_file_torrent) {
        return d->tc->getTorrentFile(d->file_index).getPathOnDisk();
    } else {
        return d->tc->getStats().output_path;
    }
}

void TorrentFileStream::chunkDownloaded(TorrentInterface *tc, Uint32 chunk)
{
    Q_UNUSED(tc);
    Q_UNUSED(chunk);
    d->update();
    Q_EMIT readyRead();
}

void TorrentFileStream::emitReadChannelFinished()
{
    Q_EMIT readChannelFinished();
}

//////////////////////////////////////////////////
TorrentFileStream::Private::Private(TorrentControl *tc, ChunkManager *cman, bool streaming_mode, TorrentFileStream *p)
    : tc(tc)
    , file_index(0)
    , cman(cman)
    , p(p)
    , current_byte_offset(0)
    , bytes_readable(0)
    , opened(false)
    , current_chunk_offset(0)
    , csel(nullptr)
    , bitset(cman->getNumChunks())
{
    current_chunk = firstChunk();
    connect(tc, &TorrentControl::chunkDownloaded, p, &TorrentFileStream::chunkDownloaded);

    if (streaming_mode) {
        csel = new StreamingChunkSelector();
        tc->setChunkSelector(csel);
        csel->setSequentialRange(firstChunk(), lastChunk());
    }
}

TorrentFileStream::Private::Private(TorrentControl *tc, Uint32 file_index, ChunkManager *cman, bool streaming_mode, TorrentFileStream *p)
    : tc(tc)
    , file_index(file_index)
    , cman(cman)
    , p(p)
    , current_byte_offset(0)
    , bytes_readable(0)
    , opened(false)
    , current_chunk_offset(0)
    , csel(nullptr)
{
    current_chunk = firstChunk();
    current_chunk_offset = firstChunkOffset();
    bitset = BitSet(lastChunk() - firstChunk() + 1);

    if (streaming_mode) {
        csel = new StreamingChunkSelector();
        tc->setChunkSelector(csel);
        csel->setSequentialRange(firstChunk(), lastChunk());
    }
}

TorrentFileStream::Private::~Private()
{
    if (csel && tc)
        tc->setChunkSelector(nullptr); // Force creation of new chunk selector
}

void TorrentFileStream::Private::reset()
{
    current_byte_offset = 0;
    bytes_readable = 0;
    current_chunk = firstChunk();
    current_chunk_offset = firstChunkOffset();
    current_chunk_data = PieceData::Ptr();
    cman->checkMemoryUsage();
    update();
}

void TorrentFileStream::Private::update()
{
    const BitSet &chunks = cman->getBitSet();
    // Update the current limit
    bt::Uint32 first = firstChunk();
    bt::Uint32 last = lastChunk();
    if (first == last) {
        // Special case
        if (chunks.get(first)) {
            bitset.set(0, true);
            bytes_readable = p->size() - current_byte_offset;
        } else {
            bitset.set(0, false);
            bytes_readable = 0;
        }
        return;
    }

    // Update the bitset
    for (Uint32 i = first; i <= last; i++)
        bitset.set(i - first, chunks.get(i));

    bytes_readable = 0;
    if (!bitset.get(current_chunk - first)) // If we don't have the current chunk, we can't read anything
        return;

    // Add all chunks past the current one
    for (Uint32 i = current_chunk + 1; i <= last; i++) {
        // If we don't have the chunk we cannot read it, so break out of loop
        if (!bitset.get(i - first))
            break;

        if (i != last)
            bytes_readable += cman->getChunk(i)->getSize();
        else
            bytes_readable += lastChunkSize();
    }

    // Finally add the bytes from the first chunk
    if (current_chunk == last)
        bytes_readable += lastChunkSize() - current_chunk_offset;
    else
        bytes_readable += cman->getChunk(current_chunk)->getSize() - current_chunk_offset;
}

Uint32 TorrentFileStream::Private::firstChunk()
{
    if (tc && tc->getStats().multi_file_torrent)
        return tc->getTorrentFile(file_index).getFirstChunk();
    else
        return 0;
}

Uint32 TorrentFileStream::Private::firstChunkOffset()
{
    if (tc && tc->getStats().multi_file_torrent)
        return tc->getTorrentFile(file_index).getFirstChunkOffset();
    else
        return 0;
}

Uint32 TorrentFileStream::Private::lastChunk()
{
    if (!tc)
        return 0;
    else if (tc->getStats().multi_file_torrent)
        return tc->getTorrentFile(file_index).getLastChunk();
    else
        return tc->getStats().total_chunks - 1;
}

Uint32 TorrentFileStream::Private::lastChunkSize()
{
    if (!tc)
        return 0;
    else if (tc->getStats().multi_file_torrent)
        return tc->getTorrentFile(file_index).getLastChunkSize();
    else
        return cman->getChunk(lastChunk())->getSize();
}

bool TorrentFileStream::Private::seek(qint64 pos)
{
    if (pos < 0 || !tc)
        return false;

    current_byte_offset = pos;
    current_chunk_offset = 0;
    current_chunk_data = PieceData::Ptr();
    if (tc->getStats().multi_file_torrent) {
        bt::Uint64 tor_byte_offset = (bt::Uint64)firstChunk() * (bt::Uint64)tc->getStats().chunk_size;
        tor_byte_offset += firstChunkOffset() + pos;
        current_chunk = tor_byte_offset / tc->getStats().chunk_size;
        current_chunk_offset = tor_byte_offset % tc->getStats().chunk_size;
    } else {
        current_chunk = current_byte_offset / tc->getStats().chunk_size;
        current_chunk_offset = current_byte_offset % tc->getStats().chunk_size;
    }

    if (csel)
        csel->setCursor(current_chunk);
    return true;
}

qint64 TorrentFileStream::Private::readData(char *data, qint64 maxlen)
{
    if (!tc)
        return 0;

    // First update so we now until how far we can go
    update();

    // Check if there is something to read
    if (bytes_readable == 0)
        return 0;

    qint64 bytes_read = 0;
    while (bytes_read < maxlen && bytes_read < (qint64)bytes_readable) {
        qint64 allowed = qMin((qint64)bytes_readable - bytes_read, maxlen - bytes_read);
        qint64 ret = readCurrentChunk(data + bytes_read, allowed);
        bytes_read += ret;
        if (ret == 0)
            break;
    }

    bytes_readable -= bytes_read;

    // Make sure we do not cache to much during streaming
    if (timer.getElapsedSinceUpdate() > 10000) {
        cman->checkMemoryUsage();
        timer.update();
    }

    return bytes_read;
}

qint64 TorrentFileStream::Private::readCurrentChunk(char *data, qint64 maxlen)
{
    if (!tc)
        return 0;

    // Out(SYS_GEN|LOG_DEBUG) << "readCurrentChunk s " << current_chunk << " " << current_chunk_offset << endl;
    Chunk *c = cman->getChunk(current_chunk);
    // First make sure we have the chunk
    if (!current_chunk_data)
        current_chunk_data = c->getPiece(0, c->getSize(), true);

    if (!current_chunk_data || !current_chunk_data->ok())
        return 0;

    // Calculate how much we can read
    qint64 allowed = c->getSize() - current_chunk_offset;
    if (allowed > maxlen)
        allowed = maxlen;

    // Out(SYS_GEN|LOG_DEBUG) << "readCurrentChunk r " << allowed << endl;

    // Copy data
    current_chunk_data->read((bt::Uint8 *)data, allowed, current_chunk_offset);

    // Update internal state
    current_byte_offset += allowed;
    current_chunk_offset += allowed;
    if (current_chunk_offset == c->getSize()) {
        // The whole chunk was read, so go to the next chunk
        current_chunk++;
        current_chunk_data = PieceData::Ptr();
        current_chunk_offset = 0;
        if (csel)
            csel->setCursor(current_chunk);
    }

    // Out(SYS_GEN|LOG_DEBUG) << "readCurrentChunk f " << current_chunk << " " << current_chunk_offset << endl;
    return allowed;
}

}
