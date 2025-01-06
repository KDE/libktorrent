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
#include <utility>
#include <interfaces/monitorinterface.h>
#include <util/error.h>
#include <util/functions.h>
#include <util/infohash.h>
#include <util/log.h>
#include <util/sha1hashgen.h>
#include <util/sha2hashgen.h>

#include <KLocalizedString>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
static QString SanitizeName(const QString &name)
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
    : chunk_size(0)
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

Torrent::Torrent(const bt::InfoHash &hash)
    : info_hash(hash)
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
}

void Torrent::load(const QByteArray &data, bool verbose)
{
    BDecoder decoder(data, verbose);
    const std::unique_ptr<BDictNode> dict = decoder.decodeDict();
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
    // save info dict
    metadata = data.mid(n->getOffset(), n->getLength());
    SHA1HashGen hg1;
    SHA1Hash hash1 = SHA1Hash(hg1.generate(metadata));
    if (meta_version == 1)
        info_hash = InfoHash(hash1, SHA2Hash());
    else {
        SHA2HashGen hg2;
        SHA2Hash hash2 = SHA2Hash(hg2.generate((const Uint8 *)metadata.data(), metadata.size()));
        info_hash = InfoHash(hash1, hash2);
    }

    loaded = true;
}

void Torrent::loadInfo(BDictNode *dict)
{
    BDictNode *info = dict->getDict(QByteArrayLiteral("info"));
    if (!info)
        throw Error(i18n("Corrupted torrent."));

    const BValueNode *m = info->getValue(QByteArrayLiteral("meta version"));
    meta_version = m ? m->data().toInt() : 1;
    if (meta_version > 2)
        throw Error(i18n("Unsupported torrent version: %1.", meta_version));

    chunk_size = info->getInt64(QByteArrayLiteral("piece length"));

    BDictNode *file_tree = info->getDict(QByteArrayLiteral("file tree"));
    BDictNode *piece_layers = dict->getDict(QByteArrayLiteral("piece layers"));
    if (meta_version >= 2 && !file_tree) {
        Out(SYS_GEN | LOG_DEBUG) << "File tree not found" << endl;
        throw Error(i18n("Corrupted torrent."));
    }
    if (meta_version >= 2 && !piece_layers) {
        Out(SYS_GEN | LOG_DEBUG) << "Piece layers not found" << endl;
        throw Error(i18n("Corrupted torrent."));
    }
    if (meta_version == 1 && file_tree) {
        throw Error(i18n("Corrupted torrent."));
    }

    BListNode *files = info->getList(QByteArrayLiteral("files"));
    if (files)
        loadFilesV1(files);
    else if (meta_version == 1)
        total_size = info->getInt64(QByteArrayLiteral("length"));

    if (meta_version >= 2)
        loadFilesV2(file_tree, piece_layers);

    if (files)
        loadHashV1(info);

    unencoded_name = info->getByteArray(QByteArrayLiteral("name"));
    name_suggestion = QString::fromUtf8(unencoded_name);
    name_suggestion = SanitizeName(name_suggestion);
    BValueNode *n = info->getValue(QByteArrayLiteral("private"));
    if (n && n->data().toInt() == 1)
        priv_torrent = true;

    if (files) {
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
}

void Torrent::loadFilesV1(BListNode *node)
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

void Torrent::loadFilesV2(BDictNode *node, BDictNode *piece_layers, Uint8 depth, QStringList path, Uint32 idx)
{
    if (!node)
        throw Error(i18n("Corrupted torrent."));

    // We parse file tree structure recursively, so make sure not to overflow the stack.
    if (depth > 100) {
        throw Error(i18n("Torrent contains too many levels of nested directories."));
    }

    const QList<QByteArray> keys = node->keys();
    for (const QByteArray &key : keys) {
        BDictNode *second = node->getDict(key);
        if (second == nullptr) {
            throw Error(i18n("Corrupted torrent."));
        }

        bool leaf_node = key.isEmpty();
        if (leaf_node) {
            Uint64 length = second->getInt64(QByteArrayLiteral("length"));
            QByteArray root = second->getByteArray(QByteArrayLiteral("pieces root"));
            if (length > chunk_size) {
                QList<SHA2Hash> hashes_list = loadHashV2(piece_layers, root); // TODO: Not yet used
            }

            QString file_path = SanitizeName(path.join(bt::DirSeparator()));
            TorrentFile file(this, idx, file_path, total_size, length, chunk_size);
            total_size += length;
            idx++;
            files.append(file);
        } else {
            QString directory_name = QString::fromUtf8(key).remove(bt::DirSeparator());
            if (directory_name.isEmpty()) {
                throw Error(i18n("Invalid directory name."));
            }
            loadFilesV2(second, piece_layers, depth + 1, QStringList(path) << directory_name);
        }
    }
}

void Torrent::loadTrackerURL(const QString &s)
{
    if (!trackers)
        trackers = std::make_unique<TrackerTier>();

    QUrl url(s);
    if (s.length() > 0 && url.isValid())
        trackers->urls.append(url);
}

void Torrent::loadHashV1(BDictNode *dict)
{
    const BValueNode *pieces = dict->getValue(QByteArrayLiteral("pieces"));
    if (!pieces) {
        if (meta_version == 1) {
            throw Error(i18n("Corrupted torrent."));
        }
        return;
    }
    const QByteArray hash_string = dict->getByteArray(QByteArrayLiteral("pieces"));
    const QByteArrayView hash_string_view{hash_string};
    for (int i = 0; i < hash_string.size(); i += 20) {
        SHA1Hash hash(hash_string_view.sliced(i, 20));
        hash_pieces.append(hash);
    }
}

QList<SHA2Hash> Torrent::loadHashV2(BDictNode *piece_layers, QByteArray root)
{
    QList<SHA2Hash> hash_pieces;
    const QByteArray hash_string = piece_layers->getByteArray(root);
    for (int i = 0; i < hash_string.size(); i += 32) {
        Uint8 h[32];
        memcpy(h, hash_string.data() + i, 32);
        SHA2Hash hash(h);
        hash_pieces.append(hash);
    }
    return hash_pieces;
}

void Torrent::loadAnnounceList(BNode *node)
{
    if (!node)
        return;

    BListNode *ml = dynamic_cast<BListNode *>(node);
    if (!ml)
        return;

    if (!trackers)
        trackers = std::make_unique<TrackerTier>();

    TrackerTier *tier = trackers.get();
    // ml->printDebugInfo();
    for (Uint32 i = 0; i < ml->getNumChildren(); i++) {
        BListNode *url_list = ml->getList(i);
        if (url_list) {
            for (Uint32 j = 0; j < url_list->getNumChildren(); j++)
                tier->urls.append(QUrl(url_list->getString(j)));
            tier->next = std::make_unique<TrackerTier>();
            tier = tier->next.get();
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
    TrackerTier *tt = trackers.get();
    while (tt) {
        count += tt->urls.count();
        tt = tt->next.get();
    }
    return count;
}

void Torrent::calcChunkPos(Uint32 chunk, FileIndexList &file_list, int max_files) const
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
            if (file_list.count() >= max_files)
                break;
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
    FileIndexList cfiles;
    calcChunkPos(chunk, cfiles);

    for (Uint32 cfindex : std::as_const(cfiles)) {
        TorrentFile &f = getFile(cfindex);
        f.updateNumDownloadedChunks(cman);
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
