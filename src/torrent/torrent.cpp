/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2005 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "torrent.h"

#include <QDataStream>
#include <QFile>
#include <QStringList>

#include <bcodec/bdecoder.h>
#include <bcodec/bnode.h>
#include <cstdlib>
#include <ctime>
#include <interfaces/monitorinterface.h>
#include <util/error.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/sha1hashgen.h>

#include <KLocalizedString>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
static QString SanityzeName(const QString &name)
{
    QString ret = name;
#ifdef Q_OS_WIN
    const QLatin1Char invalid[] = {'<'_L1, '>'_L1, ':'_L1, '"'_L1, '/'_L1, '\\'_L1, '|'_L1, '?'_L1, '*'_L1};
    for (int i = 0; i < 9; i++) {
        if (ret.contains(invalid[i]))
            ret = ret.replace(invalid[i], '_'_L1);
    }
#else
    if (ret.endsWith('/'_L1))
        ret.chop(1);
    if (ret.startsWith('/'_L1))
        ret = ret.mid(1);
#endif
    // Don't allow directory traversal things in names
    if (ret.contains('/'_L1) || ret.contains(".."_L1)) {
        QStringList sl = ret.split(bt::DirSeparator());
        sl.removeAll(QLatin1String(".."));
        ret = sl.join(QStringLiteral("_"));
    }

    return ret;
}

Torrent::Torrent()
    : trackers(nullptr)
    , chunk_size(0)
    , last_chunk_size(0)
    , total_size(0)
    , file_prio_listener(nullptr)
    , pos_cache_chunk(0)
    , pos_cache_file(0)
    , tmon(nullptr)
    , priv_torrent(false)
    , loaded(false)
{
}

Torrent::Torrent(const bt::SHA1Hash &hash)
    : info_hash(hash)
    , trackers(nullptr)
    , chunk_size(0)
    , last_chunk_size(0)
    , total_size(0)
    , file_prio_listener(nullptr)
    , pos_cache_chunk(0)
    , pos_cache_file(0)
    , tmon(nullptr)
    , priv_torrent(false)
    , loaded(false)
{
}

Torrent::~Torrent()
{
    delete trackers;
}

void Torrent::load(const QByteArray &data, bool verbose)
{
    BNode *node = nullptr;

    try {
        BDecoder decoder(data, verbose);
        node = decoder.decode();
        BDictNode *dict = dynamic_cast<BDictNode *>(node);
        if (!dict)
            throw Error(i18n("Corrupted torrent."));

        BValueNode *c = dict->getValue(QByteArrayLiteral("comment"));
        if (c)
            comments = c->data().toString();

        const BValueNode *announce = dict->getValue(QByteArrayLiteral("announce"));
        BListNode *nodes = dict->getList(QByteArrayLiteral("nodes"));
        // if (!announce && !nodes)
        //  throw Error(i18n("Torrent has no announce or nodes field."));

        if (announce)
            loadTrackerURL(dict->getString(QByteArrayLiteral("announce")));

        if (nodes) // DHT torrrents have a node key
            loadNodes(nodes);

        loadInfo(dict->getDict(QByteArrayLiteral("info")));
        loadAnnounceList(dict->getData(QByteArrayLiteral("announce-list")));

        // see if the torrent contains webseeds
        BListNode *urls = dict->getList(QByteArrayLiteral("url-list"));
        if (urls) {
            loadWebSeeds(urls);
        } else if (dict->getValue(QByteArrayLiteral("url-list"))) {
            QUrl url(dict->getString(QByteArrayLiteral("url-list")));
            if (url.isValid())
                web_seeds.append(url);
        }

        BNode *n = dict->getData(QByteArrayLiteral("info"));
        SHA1HashGen hg;
        // save info dict
        metadata = data.mid(n->getOffset(), n->getLength());
        info_hash = hg.generate((const Uint8 *)metadata.data(), metadata.size());
        delete node;

        loaded = true;
    } catch (...) {
        delete node;
        throw;
    }
}

void Torrent::loadInfo(BDictNode *dict)
{
    if (!dict)
        throw Error(i18n("Corrupted torrent."));

    chunk_size = dict->getInt64(QByteArrayLiteral("piece length"));
    BListNode *files = dict->getList(QByteArrayLiteral("files"));
    if (files)
        loadFiles(files);
    else
        total_size = dict->getInt64(QByteArrayLiteral("length"));

    loadHash(dict);
    unencoded_name = dict->getByteArray(QByteArrayLiteral("name"));
    name_suggestion = QString::fromUtf8(unencoded_name);
    name_suggestion = SanityzeName(name_suggestion);
    BValueNode *n = dict->getValue(QByteArrayLiteral("private"));
    if (n && n->data().toInt() == 1)
        priv_torrent = true;

    // do a safety check to see if the number of hashes matches the file_length
    Uint32 num_chunks = (total_size / chunk_size);
    last_chunk_size = total_size % chunk_size;
    if (last_chunk_size > 0)
        num_chunks++;
    else
        last_chunk_size = chunk_size;

    if (num_chunks != (Uint32)hash_pieces.count()) {
        Out(SYS_GEN | LOG_DEBUG) << "File sizes and number of hashes do not match for " << name_suggestion << endl;
        throw Error(i18n("Corrupted torrent."));
    }
}

void Torrent::loadFiles(BListNode *node)
{
    if (!node)
        throw Error(i18n("Corrupted torrent."));

    Uint32 idx = 0;
    BListNode *fl = node;
    for (Uint32 i = 0; i < fl->getNumChildren(); i++) {
        BDictNode *d = fl->getDict(i);
        if (!d)
            throw Error(i18n("Corrupted torrent."));

        BListNode *ln = d->getList(QByteArrayLiteral("path"));
        if (!ln)
            throw Error(i18n("Corrupted torrent."));

        QString path;
        QList<QByteArray> unencoded_path;
        for (Uint32 j = 0; j < ln->getNumChildren(); j++) {
            QByteArray v = ln->getByteArray(j);
            unencoded_path.append(v);
            QString sd = QString::fromUtf8(v);
            if (sd.contains('\n'_L1))
                sd = sd.remove('\n'_L1);
            path += sd;
            if (j + 1 < ln->getNumChildren())
                path += bt::DirSeparator();
        }

        // we do not want empty dirs
        if (path.endsWith(bt::DirSeparator()))
            continue;

        if (!checkPathForDirectoryTraversal(path))
            throw Error(i18n("Corrupted torrent."));

        Uint64 s = d->getInt64(QByteArrayLiteral("length"));
        TorrentFile file(this, idx, path, total_size, s, chunk_size);
        file.setUnencodedPath(unencoded_path);

        // update file_length
        total_size += s;
        files.append(file);
        idx++;
    }
}

void Torrent::loadTrackerURL(const QString &s)
{
    if (!trackers)
        trackers = new TrackerTier();

    QUrl url(s);
    if (s.length() > 0 && url.isValid())
        trackers->urls.append(url);
}

void Torrent::loadHash(BDictNode *dict)
{
    QByteArray hash_string = dict->getByteArray(QByteArrayLiteral("pieces"));
    for (int i = 0; i < hash_string.size(); i += 20) {
        Uint8 h[20];
        memcpy(h, hash_string.data() + i, 20);
        SHA1Hash hash(h);
        hash_pieces.append(hash);
    }
}

void Torrent::loadAnnounceList(BNode *node)
{
    if (!node)
        return;

    BListNode *ml = dynamic_cast<BListNode *>(node);
    if (!ml)
        return;

    if (!trackers)
        trackers = new TrackerTier();

    TrackerTier *tier = trackers;
    // ml->printDebugInfo();
    for (Uint32 i = 0; i < ml->getNumChildren(); i++) {
        BListNode *url_list = ml->getList(i);
        if (url_list) {
            for (Uint32 j = 0; j < url_list->getNumChildren(); j++)
                tier->urls.append(QUrl(url_list->getString(j)));
            tier->next = new TrackerTier();
            tier = tier->next;
        }
    }
}

void Torrent::loadNodes(BListNode *node)
{
    for (Uint32 i = 0; i < node->getNumChildren(); i++) {
        BListNode *c = node->getList(i);
        if (!c || c->getNumChildren() != 2)
            throw Error(i18n("Corrupted torrent."));

        // first child is the IP, second the port
        // add the DHT node
        DHTNode n;
        n.ip = c->getString(0);
        n.port = c->getInt(1);
        nodes.append(n);
    }
}

void Torrent::loadWebSeeds(BListNode *node)
{
    for (Uint32 i = 0; i < node->getNumChildren(); i++) {
        QUrl url = QUrl(node->getString(i));
        if (url.isValid())
            web_seeds.append(url);
    }
}

void Torrent::debugPrintInfo()
{
    Out(SYS_GEN | LOG_DEBUG) << "Name : " << name_suggestion << endl;

    //      for (cost QUrl & url : std::as_const(tracker_urls) )
    //          Out(SYS_GEN|LOG_DEBUG) << "Tracker URL : " << url << endl;

    Out(SYS_GEN | LOG_DEBUG) << "Piece Length : " << chunk_size << endl;
    if (this->isMultiFile()) {
        Out(SYS_GEN | LOG_DEBUG) << "Files : " << endl;
        Out(SYS_GEN | LOG_DEBUG) << "===================================" << endl;
        for (Uint32 i = 0; i < getNumFiles(); i++) {
            TorrentFile &tf = getFile(i);
            Out(SYS_GEN | LOG_DEBUG) << "Path : " << tf.getPath() << endl;
            Out(SYS_GEN | LOG_DEBUG) << "Size : " << tf.getSize() << endl;
            Out(SYS_GEN | LOG_DEBUG) << "First Chunk : " << tf.getFirstChunk() << endl;
            Out(SYS_GEN | LOG_DEBUG) << "Last Chunk : " << tf.getLastChunk() << endl;
            Out(SYS_GEN | LOG_DEBUG) << "First Chunk Off : " << tf.getFirstChunkOffset() << endl;
            Out(SYS_GEN | LOG_DEBUG) << "Last Chunk Size : " << tf.getLastChunkSize() << endl;
            Out(SYS_GEN | LOG_DEBUG) << "===================================" << endl;
        }
    } else {
        Out(SYS_GEN | LOG_DEBUG) << "File Length : " << total_size << endl;
    }
    Out(SYS_GEN | LOG_DEBUG) << "Pieces : " << hash_pieces.size() << endl;
}

bool Torrent::verifyHash(const SHA1Hash &h, Uint32 index)
{
    if (index >= (Uint32)hash_pieces.count())
        return false;

    const SHA1Hash &ph = hash_pieces[index];
    return ph == h;
}

const SHA1Hash &Torrent::getHash(Uint32 idx) const
{
    if (idx >= (Uint32)hash_pieces.count())
        throw Error(u"Torrent::getHash %1 is out of bounds"_s.arg(idx));

    return hash_pieces[idx];
}

TorrentFile &Torrent::getFile(Uint32 idx)
{
    if (idx >= (Uint32)files.size())
        return TorrentFile::null;

    return files[idx];
}

const TorrentFile &Torrent::getFile(Uint32 idx) const
{
    if (idx >= (Uint32)files.size())
        return TorrentFile::null;

    return files.at(idx);
}

unsigned int Torrent::getNumTrackerURLs() const
{
    Uint32 count = 0;
    TrackerTier *tt = trackers;
    while (tt) {
        count += tt->urls.count();
        tt = tt->next;
    }
    return count;
}

void Torrent::calcChunkPos(Uint32 chunk, QList<Uint32> &file_list) const
{
    file_list.clear();
    if (chunk >= (Uint32)hash_pieces.size() || files.empty())
        return;

    int start = (chunk >= this->pos_cache_chunk) ? this->pos_cache_file : 0;
    int end = (files.count() - 1);
    int mid = start + (end - start) / 2;
    while (start != mid && mid != end) {
        // printf("start = %i ; end = %i ; mid = %i\n",start,end,mid);
        const TorrentFile &f = files[mid];
        if (chunk >= f.getFirstChunk() && chunk <= f.getLastChunk()) {
            int i = mid;
            while (i > 0) {
                i--;
                const TorrentFile &tf = files[i];
                if (!(chunk >= tf.getFirstChunk() && chunk <= tf.getLastChunk())) {
                    i++;
                    break;
                }
            }
            mid = i;
            break;
        } else {
            if (chunk > f.getLastChunk()) {
                // chunk comes after file
                start = mid + 1;
                mid = start + (end - start) / 2;
            } else {
                // chunk comes before file
                end = mid - 1;
                mid = start + (end - start) / 2;
            }
        }
    }

    for (int i = mid; i < files.count(); i++) {
        const TorrentFile &f = files[i];
        if (chunk >= f.getFirstChunk() && chunk <= f.getLastChunk() && f.getSize() != 0) {
            file_list.append(f.getIndex());
        } else if (chunk < f.getFirstChunk())
            break;
    }

    pos_cache_chunk = chunk;
    pos_cache_file = file_list.at(0);
}

bool Torrent::isMultimedia() const
{
    return IsMultimediaFile(this->getNameSuggestion());
}

void Torrent::updateFilePercentage(ChunkManager &cman)
{
    for (int i = 0; i < files.count(); i++) {
        TorrentFile &f = files[i];
        f.updateNumDownloadedChunks(cman);
    }
}

void Torrent::updateFilePercentage(Uint32 chunk, ChunkManager &cman)
{
    QList<Uint32> cfiles;
    calcChunkPos(chunk, cfiles);

    QList<Uint32>::iterator i = cfiles.begin();
    while (i != cfiles.end()) {
        TorrentFile &f = getFile(*i);
        f.updateNumDownloadedChunks(cman);
        ++i;
    }
}

bool Torrent::checkPathForDirectoryTraversal(const QString &p)
{
    QStringList sl = p.split(bt::DirSeparator());
    return !sl.contains(QLatin1String(".."));
}

void Torrent::downloadPriorityChanged(TorrentFile *tf, Priority newpriority, Priority oldpriority)
{
    if (file_prio_listener)
        file_prio_listener->downloadPriorityChanged(tf, newpriority, oldpriority);
}

void Torrent::filePercentageChanged(TorrentFile *tf, float perc)
{
    if (tmon)
        tmon->filePercentageChanged(tf, perc);
}

void Torrent::filePreviewChanged(TorrentFile *tf, bool preview)
{
    if (tmon)
        tmon->filePreviewChanged(tf, preview);
}
}
