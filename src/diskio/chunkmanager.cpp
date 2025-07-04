/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2005 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "chunkmanager.h"
#include <algorithm>

#include "multifilecache.h"
#include "singlefilecache.h"
#include <KLocalizedString>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>
#include <interfaces/cachefactory.h>
#include <torrent/torrent.h>
#include <util/array.h>
#include <util/bitset.h>
#include <util/error.h>
#include <util/file.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/timer.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
Uint32 ChunkManager::preview_size_audio = 256 * 1024; // 256 KB for audio files
Uint32 ChunkManager::preview_size_video = 2048 * 1024; // 2 MB for videos

class ChunkManager::Private
{
public:
    Private(ChunkManager *p, Torrent &tor, const QString &tmpdir, const QString &datadir, bool custom_output_name, CacheFactory *fac);
    ~Private();

    void saveIndexFile();
    void writeIndexFileEntry(Chunk *c);
    void saveFileInfo();
    void loadFileInfo();
    void savePriorityInfo();
    void loadPriorityInfo();
    void doPreviewPriority(TorrentFile &tf);
    bool allFilesExistOfChunk(Uint32 idx);
    bool isBorderChunk(Uint32 idx) const
    {
        return border_chunks.contains(idx);
    }
    void setBorderChunkPriority(Uint32 idx, Priority prio);
    bool resetBorderChunk(Uint32 idx, TorrentFile *tf);
    void createBorderChunkSet();
    void dumpPriority(TorrentFile *tf);
    void downloadStatusChanged(TorrentFile *tf, bool download);
    void loadIndexFile();
    void setupPriorities();

public:
    ChunkManager *p;
    QString index_file, file_info_file, file_priority_file;
    std::vector<Chunk *> chunks;
    Cache *cache;
    BitSet todo;
    mutable Uint32 chunks_left;
    mutable bool recalc_chunks_left;
    bool during_load;
    QSet<Uint32> border_chunks;
};

ChunkManager::ChunkManager(Torrent &tor, const QString &tmpdir, const QString &datadir, bool custom_output_name, CacheFactory *fac)
    : d(std::make_unique<Private>(this, tor, tmpdir, datadir, custom_output_name, fac))
    , tor(tor)
    , bitset(tor.getNumChunks())
    , excluded_chunks(tor.getNumChunks())
    , only_seed_chunks(tor.getNumChunks())
{
    d->setupPriorities();
}

ChunkManager::~ChunkManager()
{
}

QString ChunkManager::getDataDir() const
{
    return d->cache->getDataDir();
}

void ChunkManager::changeDataDir(const QString &data_dir)
{
    d->cache->changeTmpDir(data_dir);
    d->index_file = data_dir + "index"_L1;
    d->file_info_file = data_dir + "file_info"_L1;
    d->file_priority_file = data_dir + "file_priority"_L1;
}

void ChunkManager::changeOutputPath(const QString &output_path)
{
    d->cache->changeOutputPath(output_path);
}

Job *ChunkManager::moveDataFiles(const QString &ndir)
{
    return d->cache->moveDataFiles(ndir);
}

void ChunkManager::moveDataFilesFinished(Job *job)
{
    d->cache->moveDataFilesFinished(job);
}

Job *ChunkManager::moveDataFiles(const QMap<TorrentFileInterface *, QString> &files)
{
    return d->cache->moveDataFiles(files);
}

void ChunkManager::moveDataFilesFinished(const QMap<TorrentFileInterface *, QString> &files, Job *job)
{
    d->cache->moveDataFilesFinished(files, job);
}

void ChunkManager::createFiles(bool check_priority)
{
    if (!bt::Exists(d->index_file)) {
        File fptr;
        fptr.open(d->index_file, u"wb"_s);
    }
    d->cache->create();

    if (check_priority) {
        d->during_load = true; // for performance reasons
        for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
            TorrentFile &tf = tor.getFile(i);
            if (tf.getPriority() != NORMAL_PRIORITY) {
                downloadPriorityChanged(&tf, tf.getPriority(), tf.getOldPriority());
            }
        }
        d->during_load = false;
        d->savePriorityInfo();
    }
}

bool ChunkManager::hasMissingFiles(QStringList &sl)
{
    return d->cache->hasMissingFiles(sl);
}

bool ChunkManager::isStorageMounted(QStringList &missing)
{
    return d->cache->isStorageMounted(missing);
}

void ChunkManager::saveFileMap()
{
    return d->cache->saveFileMap();
}

Chunk *ChunkManager::getChunk(unsigned int i)
{
    if (i >= (Uint32)d->chunks.size())
        return nullptr;
    else
        return d->chunks[i];
}

void ChunkManager::start()
{
    d->cache->open();
}

void ChunkManager::stop()
{
    d->cache->close();
}

void ChunkManager::resetChunk(unsigned int i)
{
    if (i >= (Uint32)d->chunks.size() || d->during_load)
        return;

    Chunk *c = d->chunks[i];
    d->cache->clearPieces(c);
    c->setStatus(Chunk::NOT_DOWNLOADED);
    bitset.set(i, false);
    d->todo.set(i, !excluded_chunks.get(i) && !only_seed_chunks.get(i));
    tor.updateFilePercentage(i, *this);
    Out(SYS_DIO | LOG_DEBUG) << u"Reset chunk %1"_s.arg(i) << endl;
}

void ChunkManager::checkMemoryUsage()
{
    d->cache->checkMemoryUsage();
}

void ChunkManager::chunkDownloaded(unsigned int i)
{
    if (i >= (Uint32)d->chunks.size())
        return;

    Chunk *c = d->chunks[i];
    if (!c->isExcluded()) {
        // update the index file
        bitset.set(i, true);
        d->todo.set(i, false);
        d->recalc_chunks_left = true;
        d->writeIndexFileEntry(c);
        c->setStatus(Chunk::ON_DISK);
        tor.updateFilePercentage(i, *this);
    } else {
        Out(SYS_DIO | LOG_IMPORTANT) << "Warning: attempted to save a chunk which was excluded" << endl;
    }
}

Uint32 ChunkManager::onlySeedChunks() const
{
    return only_seed_chunks.numOnBits();
}

bool ChunkManager::completed() const
{
    return d->todo.numOnBits() == 0 && bitset.numOnBits() > 0;
}

Uint64 ChunkManager::bytesLeft() const
{
    Uint32 num_left = bitset.getNumBits() - bitset.numOnBits();
    Uint32 last = d->chunks.size() - 1;
    if (last < (Uint32)d->chunks.size() && !bitset.get(last)) {
        Chunk *c = d->chunks[last];
        if (c)
            return (num_left - 1) * tor.getChunkSize() + c->getSize();
        else
            return num_left * tor.getChunkSize();
    } else {
        return num_left * tor.getChunkSize();
    }
}

Uint64 ChunkManager::bytesLeftToDownload() const
{
    Uint32 num_left = d->todo.numOnBits();
    Uint32 last = d->chunks.size() - 1;
    if (last < (Uint32)d->chunks.size() && d->todo.get(last)) {
        Chunk *c = d->chunks[last];
        if (c)
            return (num_left - 1) * tor.getChunkSize() + c->getSize();
        else
            return num_left * tor.getChunkSize();
    } else {
        return num_left * tor.getChunkSize();
    }
}

Uint32 ChunkManager::chunksLeft() const
{
    if (!d->recalc_chunks_left)
        return d->chunks_left;

    Uint32 num = 0;
    Uint32 tot = d->chunks.size();
    for (Uint32 i = 0; i < tot; i++) {
        const Chunk *c = d->chunks[i];
        if (c && !bitset.get(i) && !c->isExcluded())
            num++;
    }
    d->chunks_left = num;
    d->recalc_chunks_left = false;
    return num;
}

bool ChunkManager::haveAllChunks() const
{
    return bitset.numOnBits() == bitset.getNumBits();
}

Uint64 ChunkManager::bytesExcluded() const
{
    Uint64 excl = 0;
    if (excluded_chunks.get(tor.getNumChunks() - 1)) {
        Chunk *c = d->chunks[tor.getNumChunks() - 1];
        Uint32 num = excluded_chunks.numOnBits() - 1;
        excl = tor.getChunkSize() * num + c->getSize();
    } else {
        excl = tor.getChunkSize() * excluded_chunks.numOnBits();
    }

    if (only_seed_chunks.get(tor.getNumChunks() - 1)) {
        Chunk *c = d->chunks[tor.getNumChunks() - 1];
        Uint32 num = only_seed_chunks.numOnBits() - 1;
        excl += tor.getChunkSize() * num + c->getSize();
    } else {
        excl += tor.getChunkSize() * only_seed_chunks.numOnBits();
    }
    return excl;
}

Uint32 ChunkManager::chunksExcluded() const
{
    return excluded_chunks.numOnBits() + only_seed_chunks.numOnBits();
}

Uint32 ChunkManager::chunksDownloaded() const
{
    return bitset.numOnBits();
}

void ChunkManager::debugPrintMemUsage()
{
    //  Out(SYS_DIO|LOG_DEBUG) << "Active Chunks : " << loaded.count()<< endl;
}

void ChunkManager::prioritise(Uint32 from, Uint32 to, Priority priority)
{
    if (from > to)
        std::swap(from, to);

    Uint32 i = from;
    while (i <= to && i < (Uint32)d->chunks.size()) {
        Chunk *c = d->chunks[i];
        c->setPriority(priority);

        if (priority == ONLY_SEED_PRIORITY) {
            only_seed_chunks.set(i, true);
            d->todo.set(i, false);
        } else if (priority == EXCLUDED) {
            only_seed_chunks.set(i, false);
            d->todo.set(i, false);
        } else {
            only_seed_chunks.set(i, false);
            d->todo.set(i, !bitset.get(i));
        }

        i++;
    }
    Q_EMIT updateStats();
}

void ChunkManager::prioritisePreview(Uint32 from, Uint32 to)
{
    if (from > to)
        std::swap(from, to);

    for (Uint32 i = from; i <= to && i < (Uint32)d->chunks.size(); ++i) {
        Chunk *c = d->chunks[i];
        Priority priority = c->getPriority();

        switch (priority) {
        case FIRST_PRIORITY:
            priority = FIRST_PREVIEW_PRIORITY;
            break;
        case NORMAL_PRIORITY:
            priority = NORMAL_PREVIEW_PRIORITY;
            break;
        case LAST_PRIORITY:
            priority = LAST_PREVIEW_PRIORITY;
            break;
        default:
            continue;
        }

        c->setPriority(priority);
    }
    Q_EMIT updateStats();
}

void ChunkManager::exclude(Uint32 from, Uint32 to)
{
    if (from > to)
        std::swap(from, to);

    Uint32 i = from;
    while (i <= to && i < (Uint32)d->chunks.size()) {
        Chunk *c = d->chunks[i];
        c->setExclude(true);
        excluded_chunks.set(i, true);
        only_seed_chunks.set(i, false);
        d->todo.set(i, false);
        bitset.set(i, false);
        i++;
    }
    d->recalc_chunks_left = true;
    Q_EMIT excluded(from, to);
    Q_EMIT updateStats();
}

void ChunkManager::include(Uint32 from, Uint32 to)
{
    if (from > to)
        std::swap(from, to);

    Uint32 i = from;
    while (i <= to && i < (Uint32)d->chunks.size()) {
        Chunk *c = d->chunks[i];
        c->setExclude(false);
        excluded_chunks.set(i, false);
        if (!bitset.get(i))
            d->todo.set(i, true);
        i++;
    }
    d->recalc_chunks_left = true;
    Q_EMIT updateStats();
    Q_EMIT included(from, to);
}

void ChunkManager::Private::savePriorityInfo()
{
    if (during_load)
        return;

    // save priority info and call saveFileInfo
    saveFileInfo();
    File fptr;
    if (!fptr.open(file_priority_file, u"wb"_s)) {
        Out(SYS_DIO | LOG_IMPORTANT) << "Warning : Can not save chunk_info file : " << fptr.errorString() << endl;
        return;
    }

    try {
        QList<Uint32> dnd;
        Torrent &tor = p->tor;
        Uint32 i = 0;
        for (; i < tor.getNumFiles(); i++) {
            if (tor.getFile(i).getPriority() != NORMAL_PRIORITY) {
                dnd.append(i);
                dnd.append(tor.getFile(i).getPriority());
            }
        }

        Uint32 tmp = dnd.count();
        fptr.write(&tmp, sizeof(Uint32));
        // write all the non-default priority ones
        for (i = 0; i < (Uint32)dnd.count(); i++) {
            tmp = dnd[i];
            fptr.write(&tmp, sizeof(Uint32));
        }
        fptr.flush();
    } catch (bt::Error &err) {
        Out(SYS_DIO | LOG_IMPORTANT) << "Failed to save priority file " << err.toString() << endl;
        bt::Delete(file_priority_file, true);
    }
}

void ChunkManager::Private::loadPriorityInfo()
{
    // load priority info and if that fails load file info
    File fptr;
    if (!fptr.open(file_priority_file, u"rb"_s)) {
        loadFileInfo();
        return;
    }

    Torrent &tor = p->tor;

    Uint32 num = 0;
    // first read the number of lines
    if (fptr.read(&num, sizeof(Uint32)) != sizeof(Uint32) || num > 2 * tor.getNumFiles()) {
        Out(SYS_DIO | LOG_IMPORTANT) << "Warning : error reading chunk_info file" << endl;
        loadFileInfo();
        return;
    }

    Array<Uint32> buf(num);
    if (fptr.read(buf, sizeof(Uint32) * num) != sizeof(Uint32) * num) {
        Out(SYS_DIO | LOG_IMPORTANT) << "Warning : error reading chunk_info file" << endl;
        loadFileInfo();
        return;
    }

    fptr.close();

    for (Uint32 i = 0; i < num; i += 2) {
        Uint32 idx = buf[i];
        if (idx >= tor.getNumFiles()) {
            Out(SYS_DIO | LOG_IMPORTANT) << "Warning : error reading chunk_info file" << endl;
            loadFileInfo();
            return;
        }

        bt::TorrentFile &tf = tor.getFile(idx);
        if (!tf.isNull()) {
            // numbers are to be compatible with old chunk info files
            switch (buf[i + 1]) {
            case FIRST_PRIORITY:
                tf.setPriority(FIRST_PRIORITY);
                break;
            case NORMAL_PRIORITY:
                // By default priority is set to normal, so do nothing
                // tf.setPriority(NORMAL_PRIORITY);
                break;
            case EXCLUDED:
                // tf.setDoNotDownload(true);
                tf.setPriority(EXCLUDED);
                break;
            case ONLY_SEED_PRIORITY:
                tf.setPriority(ONLY_SEED_PRIORITY);
                break;
            default:
                tf.setPriority(LAST_PRIORITY);
                break;
            }
        }
    }
}

void ChunkManager::downloadPriorityChanged(TorrentFile *tf, Priority newpriority, Priority oldpriority)
{
    if (newpriority == EXCLUDED) {
        d->downloadStatusChanged(tf, false);
        //  dumpPriority(tf);
        return;
    }

    if (oldpriority == EXCLUDED) {
        d->downloadStatusChanged(tf, true);
    }

    d->savePriorityInfo();

    Uint32 first = tf->getFirstChunk();
    Uint32 last = tf->getLastChunk();

    if (oldpriority == ONLY_SEED_PRIORITY)
        include(first, last);

    if (first == last) {
        if (d->isBorderChunk(first))
            d->setBorderChunkPriority(first, newpriority);
        else
            prioritise(first, first, newpriority);

        if (newpriority == ONLY_SEED_PRIORITY)
            Q_EMIT excluded(first, last);
    } else {
        // if the first one is a border chunk use setBorderChunkPriority and make the range smaller
        if (d->isBorderChunk(first)) {
            d->setBorderChunkPriority(first, newpriority);
            first++;
        }

        // if the last one is a border chunk use setBorderChunkPriority and make the range smaller
        if (d->isBorderChunk(last)) {
            d->setBorderChunkPriority(last, newpriority);
            last--;
        }

        // if we still have a valid range, prioritise it
        if (first <= last) {
            prioritise(first, last, newpriority);
            if (newpriority == ONLY_SEED_PRIORITY)
                Q_EMIT excluded(first, last);
        }
    }

    // if it is a multimedia file, make sure we haven't overridden preview priority
    if (tf->isMultimedia()) {
        d->doPreviewPriority(*tf);
    }
    // dumpPriority(tf);
}

QString ChunkManager::getOutputPath() const
{
    return d->cache->getOutputPath();
}

void ChunkManager::preparePreallocation(PreallocationThread *prealloc)
{
    d->cache->preparePreallocation(prealloc);
}

void ChunkManager::dataChecked(const bt::BitSet &ok_chunks, bt::Uint32 from, bt::Uint32 to)
{
    // go over all chunks at check each of them
    for (Uint32 i = from; i < (Uint32)d->chunks.size() && i <= to; i++) {
        Chunk *c = d->chunks[i];
        if (ok_chunks.get(i) && !bitset.get(i)) {
            // We think we do not have a chunk, but we do have it
            bitset.set(i, true);
            d->todo.set(i, false);
            // the chunk must be on disk
            c->setStatus(Chunk::ON_DISK);
            tor.updateFilePercentage(i, *this);
        } else if (!ok_chunks.get(i) && bitset.get(i)) {
            Out(SYS_DIO | LOG_IMPORTANT) << "Previously OK chunk " << i << " is corrupt !!!!!" << endl;
            // We think we have a chunk, but we don't
            bitset.set(i, false);
            d->todo.set(i, !only_seed_chunks.get(i) && !excluded_chunks.get(i));
            if (c->getStatus() == Chunk::ON_DISK) {
                c->setStatus(Chunk::NOT_DOWNLOADED);
                tor.updateFilePercentage(i, *this);
            } else {
                tor.updateFilePercentage(i, *this);
            }
        }
    }
    d->recalc_chunks_left = true;
    try {
        d->saveIndexFile();
    } catch (bt::Error &err) {
        Out(SYS_DIO | LOG_DEBUG) << "Failed to save index file : " << err.toString() << endl;
    } catch (...) {
        Out(SYS_DIO | LOG_DEBUG) << "Failed to save index file : unknown exception" << endl;
    }
    chunksLeft();
}

bool ChunkManager::hasExistingFiles() const
{
    return d->cache->hasExistingFiles();
}

void ChunkManager::markExistingFilesAsDownloaded()
{
    if (tor.isMultiFile()) {
        // loop over all files and mark all chunks of all existing files as
        // downloaded
        for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
            TorrentFile &tf = tor.getFile(i);
            if (!tf.isPreExistingFile())
                continue;

            // all the chunks in the middle of the file are OK
            for (Uint32 j = tf.getFirstChunk() + 1; j < tf.getLastChunk(); j++) {
                Chunk *c = d->chunks[j];
                c->setStatus(Chunk::ON_DISK);
                bitset.set(j, true);
                d->todo.set(j, false);
                tor.updateFilePercentage(j, *this);
            }

            // all files of the first chunk must be preexisting
            if (d->allFilesExistOfChunk(tf.getFirstChunk())) {
                Uint32 idx = tf.getFirstChunk();
                Chunk *c = d->chunks[idx];
                c->setStatus(Chunk::ON_DISK);
                bitset.set(idx, true);
                d->todo.set(idx, false);
                tor.updateFilePercentage(idx, *this);
            }

            // all files of the last chunk must be preexisting
            if (d->allFilesExistOfChunk(tf.getLastChunk())) {
                Uint32 idx = tf.getLastChunk();
                Chunk *c = d->chunks[idx];
                c->setStatus(Chunk::ON_DISK);
                bitset.set(idx, true);
                d->todo.set(idx, false);
                tor.updateFilePercentage(idx, *this);
            }
        }
    } else if (d->cache->hasExistingFiles()) {
        for (Uint32 i = 0; i < d->chunks.size(); i++) {
            Chunk *c = d->chunks[i];
            c->setStatus(Chunk::ON_DISK);
            bitset.set(i, true);
            d->todo.set(i, false);
            tor.updateFilePercentage(i, *this);
        }
    }

    d->recalc_chunks_left = true;
    try {
        d->saveIndexFile();
    } catch (bt::Error &err) {
        Out(SYS_DIO | LOG_DEBUG) << "Failed to save index file : " << err.toString() << endl;
    } catch (...) {
        Out(SYS_DIO | LOG_DEBUG) << "Failed to save index file : unknown exception" << endl;
    }
    chunksLeft();
}

void ChunkManager::recreateMissingFiles()
{
    createFiles();
    if (tor.isMultiFile()) {
        // loop over all files and mark all chunks of all missing files as
        // not downloaded
        for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
            TorrentFile &tf = tor.getFile(i);
            if (!tf.isMissing())
                continue;

            for (Uint32 j = tf.getFirstChunk(); j <= tf.getLastChunk(); j++)
                resetChunk(j);
            tf.setMissing(false);
        }
    } else {
        // reset all chunks in case of single file torrent
        for (Uint32 j = 0; j < tor.getNumChunks(); j++)
            resetChunk(j);
    }
    d->saveIndexFile();
    d->recalc_chunks_left = true;
    chunksLeft();
}

void ChunkManager::dndMissingFiles()
{
    //  createFiles(); // create them again
    // loop over all files and mark all chunks of all missing files as
    // not downloaded
    for (Uint32 i = 0; i < tor.getNumFiles(); i++) {
        TorrentFile &tf = tor.getFile(i);
        if (!tf.isMissing())
            continue;

        for (Uint32 j = tf.getFirstChunk(); j <= tf.getLastChunk(); j++)
            resetChunk(j);
        tf.setMissing(false);
        tf.setDoNotDownload(true); // set do not download
    }
    d->savePriorityInfo();
    d->saveIndexFile();
    d->recalc_chunks_left = true;
    chunksLeft();
}

Job *ChunkManager::deleteDataFiles()
{
    return d->cache->deleteDataFiles();
}

Uint64 ChunkManager::diskUsage()
{
    return d->cache->diskUsage();
}

Uint32 ChunkManager::previewChunkRangeSize(const TorrentFile &file) const
{
    if (!file.isMultimedia())
        return 0;

    if (file.getFirstChunk() == file.getLastChunk())
        return 1;

    Uint32 preview_size = 0;
    if (file.isVideo())
        preview_size = preview_size_video;
    else
        preview_size = preview_size_audio;

    Uint32 nchunks = preview_size / tor.getChunkSize();
    if (nchunks == 0)
        nchunks = 1;

    return nchunks;
}

Uint32 ChunkManager::previewChunkRangeSize() const
{
    QMimeType ptr = QMimeDatabase().mimeTypeForFile(tor.getNameSuggestion());
    Uint32 preview_size = 0;
    if (ptr.name().startsWith(QLatin1String("video")))
        preview_size = preview_size_video;
    else
        preview_size = preview_size_audio;

    Uint32 nchunks = preview_size / tor.getChunkSize();
    if (nchunks == 0)
        nchunks = 1;
    return nchunks;
}

void ChunkManager::setPreviewSizes(Uint32 audio, Uint32 video)
{
    preview_size_audio = audio;
    preview_size_video = video;
}

void ChunkManager::loadIndexFile()
{
    d->loadIndexFile();
    d->cache->loadMountPoints();
}

/////////////////////////////////////////////////////////////////////

ChunkManager::Private::Private(ChunkManager *p, Torrent &tor, const QString &tmpdir, const QString &datadir, bool custom_output_name, CacheFactory *fac)
    : p(p)
    , chunks(tor.getNumChunks())
    , todo(tor.getNumChunks())
{
    during_load = false;
    todo.setAll(true);

    if (!fac) {
        if (tor.isMultiFile())
            cache = new MultiFileCache(tor, tmpdir, datadir, custom_output_name);
        else
            cache = new SingleFileCache(tor, tmpdir, datadir);
    } else
        cache = fac->create(tor, tmpdir, datadir);

    cache->loadFileMap();

    index_file = tmpdir + QLatin1String("index");
    file_info_file = tmpdir + QLatin1String("file_info");
    file_priority_file = tmpdir + QLatin1String("file_priority");
    Uint64 tsize = tor.getTotalSize(); // total size
    Uint64 csize = tor.getChunkSize(); // chunk size
    Uint64 lsize = tsize - (csize * (tor.getNumChunks() - 1)); // size of last chunk

    for (Uint32 i = 0; i < tor.getNumChunks(); i++) {
        if (i + 1 < tor.getNumChunks())
            chunks[i] = new Chunk(i, csize, cache);
        else
            chunks[i] = new Chunk(i, lsize, cache);
    }

    chunks_left = 0;
    recalc_chunks_left = true;
}

ChunkManager::Private::~Private()
{
    qDeleteAll(chunks.begin(), chunks.end());
    delete cache;
}

void ChunkManager::Private::setupPriorities()
{
    Torrent &tor = p->tor;
    if (tor.isMultiFile())
        createBorderChunkSet();

    if (tor.isMultiFile()) {
        Uint32 nfiles = tor.getNumFiles();
        for (Uint32 i = 0; i < nfiles; i++) {
            TorrentFile &tf = tor.getFile(i);
            if (tf.isMultimedia())
                doPreviewPriority(tf);
        }
    } else if (tor.isMultimedia()) {
        Uint32 nchunks = p->previewChunkRangeSize();

        p->prioritisePreview(0, nchunks);
        if (tor.getNumChunks() > nchunks) {
            p->prioritisePreview(tor.getNumChunks() - nchunks, tor.getNumChunks() - 1);
        }
    }
}

void ChunkManager::Private::saveIndexFile()
{
    File fptr;
    if (!fptr.open(index_file, u"wb"_s))
        throw Error(i18n("Cannot open index file %1: %2", index_file, fptr.errorString()));

    for (unsigned int i = 0; i < p->getNumChunks(); i++) {
        Chunk *c = p->getChunk(i);
        if (c->getStatus() != Chunk::NOT_DOWNLOADED) {
            NewChunkHeader hdr;
            hdr.index = i;
            fptr.write(&hdr, sizeof(NewChunkHeader));
        }
    }
    savePriorityInfo();
}

void ChunkManager::Private::writeIndexFileEntry(Chunk *c)
{
    File fptr;
    if (!fptr.open(index_file, u"r+b"_s)) {
        // no index file, so assume it's empty
        bt::Touch(index_file, true);
        Out(SYS_DIO | LOG_IMPORTANT) << "Can not open index file : " << fptr.errorString() << endl;
        // try again
        if (!fptr.open(index_file, u"r+b"_s))
            // panick if it failes
            throw Error(i18n("Cannot open index file %1: %2", index_file, fptr.errorString()));
    }

    fptr.seek(File::END, 0);
    NewChunkHeader hdr;
    hdr.index = c->getIndex();
    fptr.write(&hdr, sizeof(NewChunkHeader));
}

void ChunkManager::Private::loadIndexFile()
{
    during_load = true;
    loadPriorityInfo();

    File fptr;
    if (!fptr.open(index_file, u"rb"_s)) {
        // no index file, so assume it's empty
        bt::Touch(index_file, true);
        Out(SYS_DIO | LOG_IMPORTANT) << "Can not open index file : " << fptr.errorString() << endl;
        during_load = false;
        return;
    }

    if (fptr.seek(File::END, 0) != 0) {
        fptr.seek(File::BEGIN, 0);

        while (!fptr.eof()) {
            NewChunkHeader hdr;
            fptr.read(&hdr, sizeof(NewChunkHeader));
            Chunk *c = p->getChunk(hdr.index);
            if (c) {
                c->setStatus(Chunk::ON_DISK);
                p->bitset.set(hdr.index, true);
                todo.set(hdr.index, false);
                recalc_chunks_left = true;
            }
        }
    }
    p->tor.updateFilePercentage(*p);
    during_load = false;
}

void ChunkManager::Private::saveFileInfo()
{
    if (during_load)
        return;

    // saves which TorrentFiles do not need to be downloaded
    File fptr;
    if (!fptr.open(file_info_file, u"wb"_s)) {
        Out(SYS_DIO | LOG_IMPORTANT) << "Warning : Can not save chunk_info file : " << fptr.errorString() << endl;
        return;
    }

    QList<Uint32> dnd;
    Torrent &tor = p->tor;
    Uint32 i = 0;
    while (i < tor.getNumFiles()) {
        if (tor.getFile(i).doNotDownload())
            dnd.append(i);
        i++;
    }

    // first write the number of excluded ones
    Uint32 tmp = dnd.count();
    fptr.write(&tmp, sizeof(Uint32));
    // then all the excluded ones
    for (i = 0; i < (Uint32)dnd.count(); i++) {
        tmp = dnd[i];
        fptr.write(&tmp, sizeof(Uint32));
    }
    fptr.flush();
}

void ChunkManager::Private::loadFileInfo()
{
    File fptr;
    if (!fptr.open(file_info_file, u"rb"_s))
        return;

    Uint32 num = 0, tmp = 0;
    // first read the number of dnd files
    if (fptr.read(&num, sizeof(Uint32)) != sizeof(Uint32)) {
        Out(SYS_DIO | LOG_IMPORTANT) << "Warning : error reading chunk_info file" << endl;
        return;
    }

    for (Uint32 i = 0; i < num; i++) {
        if (fptr.read(&tmp, sizeof(Uint32)) != sizeof(Uint32)) {
            Out(SYS_DIO | LOG_IMPORTANT) << "Warning : error reading chunk_info file" << endl;
            return;
        }

        bt::TorrentFile &tf = p->tor.getFile(tmp);
        if (!tf.isNull()) {
            Out(SYS_DIO | LOG_DEBUG) << "Excluding : " << tf.getPath() << endl;
            tf.setDoNotDownload(true);
        }
    }
}

void ChunkManager::Private::doPreviewPriority(TorrentFile &file)
{
    if (file.getPriority() == EXCLUDED || file.getPriority() == ONLY_SEED_PRIORITY)
        return;

    if (file.getFirstChunk() == file.getLastChunk()) {
        // prioritise whole file
        p->prioritisePreview(file.getFirstChunk(), file.getLastChunk());
        return;
    }

    Uint32 nchunks = p->previewChunkRangeSize(file);
    if (!nchunks)
        return;

    p->prioritisePreview(file.getFirstChunk(), file.getFirstChunk() + nchunks);
    if (file.getLastChunk() - file.getFirstChunk() > nchunks) {
        p->prioritisePreview(file.getLastChunk() - nchunks, file.getLastChunk());
    }
}

bool ChunkManager::Private::allFilesExistOfChunk(Uint32 idx)
{
    Torrent::FileIndexList files;
    Torrent &tor = p->tor;
    tor.calcChunkPos(idx, files);
    for (Uint32 fidx : std::as_const(files)) {
        TorrentFile &tf = tor.getFile(fidx);
        if (!tf.isPreExistingFile())
            return false;
    }
    return true;
}

void ChunkManager::Private::dumpPriority(TorrentFile *tf)
{
    Uint32 first = tf->getFirstChunk();
    Uint32 last = tf->getLastChunk();
    Out(SYS_DIO | LOG_DEBUG) << "DumpPriority : " << tf->getPath() << " " << first << " " << last << endl;
    for (Uint32 i = first; i <= last; i++) {
        QString prio;
        switch (chunks[i]->getPriority()) {
        case FIRST_PREVIEW_PRIORITY:
            prio = u"First (Preview)"_s;
            break;
        case FIRST_PRIORITY:
            prio = u"First"_s;
            break;
        case NORMAL_PREVIEW_PRIORITY:
            prio = u"Normal (Preview)"_s;
            break;
        case NORMAL_PRIORITY:
            prio = u"Normal"_s;
            break;
        case LAST_PREVIEW_PRIORITY:
            prio = u"Last (Preview)"_s;
            break;
        case LAST_PRIORITY:
            prio = u"Last"_s;
            break;
        case ONLY_SEED_PRIORITY:
            prio = u"Only Seed"_s;
            break;
        case EXCLUDED:
            prio = u"Excluded"_s;
            break;
        default:
            prio = u"(invalid)"_s;
            break;
        }
        Out(SYS_DIO | LOG_DEBUG) << i << " prio " << prio << endl;
    }
}

void ChunkManager::Private::setBorderChunkPriority(Uint32 idx, Priority prio)
{
    Torrent::FileIndexList files;
    Torrent &tor = p->tor;
    Priority highest = prio;
    // get list of files where first chunk lies in
    tor.calcChunkPos(idx, files);
    for (Uint32 file : std::as_const(files)) {
        Priority np = tor.getFile(file).getPriority();
        if (np > highest)
            highest = np;
    }
    p->prioritise(idx, idx, highest);
    if (highest == ONLY_SEED_PRIORITY)
        Q_EMIT p->excluded(idx, idx);
}

bool ChunkManager::Private::resetBorderChunk(Uint32 idx, TorrentFile *tf)
{
    Torrent::FileIndexList files;
    Torrent &tor = p->tor;
    tor.calcChunkPos(idx, files);
    for (Uint32 file : std::as_const(files)) {
        const TorrentFile &other = tor.getFile(file);
        if (file == tf->getIndex())
            continue;

        // This file needs to be downloaded, so we can't reset the chunk
        if (!other.doNotDownload()) {
            // Priority might need to be modified, so set it's priority
            // to the maximum of all the files who still need it
            setBorderChunkPriority(idx, other.getPriority());
            return false;
        }
    }

    // we can reset safely
    p->resetChunk(idx);
    return true;
}

void ChunkManager::Private::createBorderChunkSet()
{
    Torrent &tor = p->tor;
    // figure out border chunks
    for (Uint32 i = 0; i < tor.getNumFiles() - 1; i++) {
        TorrentFile &a = tor.getFile(i);
        TorrentFile &b = tor.getFile(i + 1);
        if (a.getLastChunk() == b.getFirstChunk())
            border_chunks.insert(a.getLastChunk());
    }
}

void ChunkManager::Private::downloadStatusChanged(TorrentFile *tf, bool download)
{
    Uint32 first = tf->getFirstChunk();
    Uint32 last = tf->getLastChunk();
    if (download) {
        // include the chunks
        p->include(first, last);

        // if it is a multimedia file, prioritise first and last chunks of file
        if (tf->isMultimedia()) {
            doPreviewPriority(*tf);
        }
    } else {
        // check for exceptional case which causes very long loops
        // check for exceptional case which causes very long loops
        if (first == last) {
            if (!isBorderChunk(first)) {
                p->resetChunk(first);
                p->exclude(first, first);
            } else {
                if (resetBorderChunk(last, tf)) // try resetting it
                    p->exclude(first, last);
            }
            cache->downloadStatusChanged(tf, download);
            savePriorityInfo();
            if (!during_load)
                p->tor.updateFilePercentage(*p);
            return;
        }

        // go over all chunks from first to last and mark them as not downloaded
        // (first and last not included)
        for (Uint32 i = first + 1; i < last; i++)
            p->resetChunk(i);

        // if the first chunk only lies in one file, reset it
        if (!isBorderChunk(first))
            p->resetChunk(first);
        else if (!resetBorderChunk(first, tf))
            // try to reset if it lies in multiple files
            first++;

        if (last != first) {
            // if the last chunk only lies in one file reset it
            if (!isBorderChunk(last))
                p->resetChunk(last);
            else if (!resetBorderChunk(last, tf))
                last--;
        }

        if (first <= last)
            p->exclude(first, last);
    }

    cache->downloadStatusChanged(tf, download);
    savePriorityInfo();
    if (!during_load)
        p->tor.updateFilePercentage(*p);
}

}

#include "moc_chunkmanager.cpp"
