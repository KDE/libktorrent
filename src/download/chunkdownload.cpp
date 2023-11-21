/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "chunkdownload.h"

#include <KLocalizedString>

#include <algorithm>

#include <diskio/chunk.h>
#include <diskio/piecedata.h>
#include <download/piece.h>
#include <interfaces/piecedownloader.h>
#include <util/array.h>
#include <util/error.h>
#include <util/file.h>
#include <util/log.h>

#include "downloader.h"

namespace bt
{
DownloadStatus::DownloadStatus()
    : timeouts(0)
{
}

DownloadStatus::~DownloadStatus()
{
}

void DownloadStatus::add(Uint32 p)
{
    status.insert(p);
}

void DownloadStatus::remove(Uint32 p)
{
    status.remove(p);
}

bool DownloadStatus::contains(Uint32 p)
{
    return status.contains(p);
}

void DownloadStatus::clear()
{
    status.clear();
}

////////////////////////////////////////////////////

ChunkDownload::ChunkDownload(Chunk *chunk)
    : chunk(chunk)
{
    num = num_downloaded = 0;
    num = chunk->getSize() / MAX_PIECE_LEN;

    if (chunk->getSize() % MAX_PIECE_LEN != 0) {
        last_size = chunk->getSize() % MAX_PIECE_LEN;
        num++;
    } else {
        last_size = MAX_PIECE_LEN;
    }

    pieces = BitSet(num);
    pieces.clear();
    piece_data = new PieceData::Ptr[num]; // array of pointers to the piece data

    dstatus.setAutoDelete(true);

    num_pieces_in_hash = 0;
    hash_gen.start();
}

ChunkDownload::~ChunkDownload()
{
    delete[] piece_data;
}

bool ChunkDownload::piece(const Piece &p, bool &ok)
{
    ok = false;
    timer.update();

    Uint32 pp = p.getOffset() / MAX_PIECE_LEN;
    Uint32 len = pp == num - 1 ? last_size : MAX_PIECE_LEN;
    if (pp >= num || pieces.get(pp) || p.getLength() != len)
        return false;

    if (DownloadStatus *ds = dstatus.find(p.getPieceDownloader()))
        ds->remove(pp);

    PieceData::Ptr buf = chunk->getPiece(p.getOffset(), p.getLength(), false);
    if (buf && buf->write(p.getData(), p.getLength()) == p.getLength()) {
        piece_data[pp] = buf;
        ok = true;
        pieces.set(pp, true);
        piece_providers.insert(p.getPieceDownloader());
        num_downloaded++;
        if (pdown.count() > 1) {
            endgameCancel(p);
        }

        updateHash();

        if (num_downloaded >= num) {
            // finalize hash
            hash_gen.end();
            releaseAllPDs();
            return true;
        }
    }

    sendRequests();
    return false;
}

void ChunkDownload::releaseAllPDs()
{
    for (PieceDownloader *pd : std::as_const(pdown)) {
        pd->release();
        sendCancels(pd);
        disconnect(pd, &PieceDownloader::timedout, this, &ChunkDownload::onTimeout);
        disconnect(pd, &PieceDownloader::rejected, this, &ChunkDownload::onRejected);
    }
    dstatus.clear();
    pdown.clear();
}

bool ChunkDownload::assign(PieceDownloader *pd)
{
    if (!pd || pdown.contains(pd))
        return false;

    pd->grab();
    pdown.append(pd);
    dstatus.insert(pd, new DownloadStatus());
    connect(pd, &PieceDownloader::timedout, this, &ChunkDownload::onTimeout);
    connect(pd, &PieceDownloader::rejected, this, &ChunkDownload::onRejected);
    sendRequests();
    return true;
}

void ChunkDownload::release(PieceDownloader *pd)
{
    if (!pdown.contains(pd))
        return;

    pd->release();
    sendCancels(pd);
    disconnect(pd, &PieceDownloader::timedout, this, &ChunkDownload::onTimeout);
    disconnect(pd, &PieceDownloader::rejected, this, &ChunkDownload::onRejected);
    dstatus.erase(pd);
    pdown.removeAll(pd);
}

void ChunkDownload::notDownloaded(const Request &r, bool reject)
{
    // find the peer
    DownloadStatus *ds = dstatus.find(r.getPieceDownloader());
    if (ds) {
        //  Out(SYS_DIO|LOG_DEBUG) << "ds != 0"  << endl;
        Uint32 p = r.getOffset() / MAX_PIECE_LEN;
        ds->remove(p);

        PieceDownloader *pd = r.getPieceDownloader();
        if (reject) {
            // reject, so release the PieceDownloader
            pd->release();
            sendCancels(pd);
            killed(pd);
        } else {
            pd->cancel(r); // cancel request
            ds->timeout();
            // if we have more then one PieceDownloader and there are timeouts, release it
            if (ds->numTimeouts() > 0 && pdown.count() > 0) {
                pd->release();
                sendCancels(pd);
                killed(pd);
            }
        }
    }

    sendRequests();
}

void ChunkDownload::onRejected(const Request &r)
{
    if (chunk->getIndex() == r.getIndex()) {
        notDownloaded(r, true);
    }
}

void ChunkDownload::onTimeout(const Request &r)
{
    // see if we are dealing with a piece of ours
    if (chunk->getIndex() == r.getIndex()) {
        Out(SYS_CON | LOG_DEBUG)
            << QString("Request timed out %1 %2 %3 %4").arg(r.getIndex()).arg(r.getOffset()).arg(r.getLength()).arg(r.getPieceDownloader()->getName()) << endl;

        notDownloaded(r, false);
    }
}

Uint32 ChunkDownload::bestPiece(PieceDownloader *pd)
{
    Uint32 best = num;
    Uint32 best_count = 0;
    // select the piece which is being downloaded the least
    for (Uint32 i = 0; i < num; i++) {
        if (pieces.get(i))
            continue;

        DownloadStatus *ds = dstatus.find(pd);
        if (ds && ds->contains(i))
            continue;

        Uint32 times_downloading = 0;
        PtrMap<PieceDownloader *, DownloadStatus>::iterator j = dstatus.begin();
        while (j != dstatus.end()) {
            if (j->first != pd && j->second->contains(i))
                times_downloading++;
            ++j;
        }

        // nobody is downloading this piece, so return it
        if (times_downloading == 0)
            return i;

        // check if the piece is better then the current best
        if (best == num || best_count > times_downloading) {
            best_count = times_downloading;
            best = i;
        }
    }

    return best;
}

void ChunkDownload::sendRequests()
{
    timer.update();
    QList<PieceDownloader *> tmp = pdown;

    while (tmp.count() > 0) {
        for (QList<PieceDownloader *>::iterator i = tmp.begin(); i != tmp.end();) {
            PieceDownloader *pd = *i;
            if (!pd->isChoked() && pd->canAddRequest() && sendRequest(pd))
                ++i;
            else
                i = tmp.erase(i);
        }
    }
}

bool ChunkDownload::sendRequest(PieceDownloader *pd)
{
    DownloadStatus *ds = dstatus.find(pd);
    if (!ds || pd->isChoked())
        return false;

    // get the best piece to download
    Uint32 bp = bestPiece(pd);
    if (bp >= num)
        return false;

    pd->download(Request(chunk->getIndex(), bp * MAX_PIECE_LEN, bp + 1 < num ? MAX_PIECE_LEN : last_size, pd));
    ds->add(bp);

    Uint32 left = num - num_downloaded;
    if (left < 2 && left > 0)
        pd->setNearlyDone(true);

    return true;
}

void ChunkDownload::update()
{
    // go over all PD's and do requets again
    sendRequests();
}

void ChunkDownload::sendCancels(PieceDownloader *pd)
{
    DownloadStatus *ds = dstatus.find(pd);
    if (!ds)
        return;

    DownloadStatus::iterator itr = ds->begin();
    while (itr != ds->end()) {
        Uint32 i = *itr;
        pd->cancel(Request(chunk->getIndex(), i * MAX_PIECE_LEN, i + 1 < num ? MAX_PIECE_LEN : last_size, nullptr));
        ++itr;
    }
    ds->clear();
    timer.update();
}

void ChunkDownload::endgameCancel(const Piece &p)
{
    auto i = pdown.constBegin();
    while (i != pdown.constEnd()) {
        PieceDownloader *pd = *i;
        DownloadStatus *ds = dstatus.find(pd);
        Uint32 pp = p.getOffset() / MAX_PIECE_LEN;
        if (ds && ds->contains(pp)) {
            pd->cancel(Request(p));
            ds->remove(pp);
        }
        i++;
    }
}

void ChunkDownload::killed(PieceDownloader *pd)
{
    if (!pdown.contains(pd))
        return;

    dstatus.erase(pd);
    pdown.removeAll(pd);
    disconnect(pd, &PieceDownloader::timedout, this, &ChunkDownload::onTimeout);
    disconnect(pd, &PieceDownloader::rejected, this, &ChunkDownload::onRejected);
}

Uint32 ChunkDownload::getChunkIndex() const
{
    return chunk->getIndex();
}

QString ChunkDownload::getPieceDownloaderName() const
{
    if (pdown.count() == 0) {
        return QString();
    } else if (pdown.count() == 1) {
        return pdown.first()->getName();
    } else {
        return i18np("1 peer", "%1 peers", pdown.count());
    }
}

Uint32 ChunkDownload::getDownloadSpeed() const
{
    Uint32 r = 0;
    for (PieceDownloader *pd : std::as_const(pdown))
        r += pd->getDownloadRate();

    return r;
}

void ChunkDownload::save(File &file)
{
    ChunkDownloadHeader hdr;
    hdr.index = chunk->getIndex();
    hdr.num_bits = pieces.getNumBits();
    hdr.buffered = true; // unused
    // save the chunk header
    file.write(&hdr, sizeof(ChunkDownloadHeader));
    // save the bitset
    file.write(pieces.getData(), pieces.getNumBytes());

    // save how many PieceHeader structs are to be written
    Uint32 num_pieces_to_follow = 0;
    for (Uint32 i = 0; i < hdr.num_bits; i++)
        if (piece_data[i] && piece_data[i]->ok())
            num_pieces_to_follow++;

    file.write(&num_pieces_to_follow, sizeof(Uint32));

    // save all buffered pieces
    for (Uint32 i = 0; i < hdr.num_bits; i++) {
        if (!piece_data[i] || !piece_data[i]->ok())
            continue;

        PieceData::Ptr pd = piece_data[i];
        PieceHeader phdr;
        phdr.piece = i;
        phdr.size = pd->length();
        phdr.mapped = pd->mapped() ? 1 : 0;
        file.write(&phdr, sizeof(PieceHeader));
        if (!pd->mapped()) { // buffered pieces need to be saved
            pd->writeToFile(file, pd->length());
        }
    }
}

bool ChunkDownload::load(File &file, ChunkDownloadHeader &hdr, bool update_hash)
{
    // read pieces
    if (hdr.num_bits != num)
        return false;

    pieces = BitSet(hdr.num_bits);
    file.read(pieces.getData(), pieces.getNumBytes());
    pieces.updateNumOnBits();

    num_downloaded = pieces.numOnBits();
    Uint32 num_pieces_to_follow = 0;
    if (file.read(&num_pieces_to_follow, sizeof(Uint32)) != sizeof(Uint32) || num_pieces_to_follow > num)
        return false;

    for (Uint32 i = 0; i < num_pieces_to_follow; i++) {
        PieceHeader phdr;
        if (file.read(&phdr, sizeof(PieceHeader)) != sizeof(PieceHeader))
            return false;

        if (phdr.piece >= num)
            return false;

        PieceData::Ptr p = chunk->getPiece(phdr.piece * MAX_PIECE_LEN, phdr.size, false);
        if (!p)
            return false;

        if (!phdr.mapped) {
            if (p->readFromFile(file, p->length()) != p->length()) {
                return false;
            }
        }
        piece_data[phdr.piece] = p;
    }

    // initialize hash
    if (update_hash) {
        num_pieces_in_hash = 0;
        updateHash();
    }

    // add a 0 downloader, so that pieces downloaded
    // in a previous session cannot get a peer banned in this session
    if (num_downloaded)
        piece_providers.insert(nullptr);

    return true;
}

Uint32 ChunkDownload::bytesDownloaded() const
{
    Uint32 num_bytes = 0;
    for (Uint32 i = 0; i < num; i++) {
        if (pieces.get(i)) {
            num_bytes += i == num - 1 ? last_size : MAX_PIECE_LEN;
        }
    }
    return num_bytes;
}

void ChunkDownload::cancelAll()
{
    auto i = pdown.constBegin();
    while (i != pdown.constEnd()) {
        sendCancels(*i);
        i++;
    }
}

PieceDownloader *ChunkDownload::getOnlyDownloader()
{
    if (piece_providers.size() == 1) {
        return *piece_providers.begin();
    } else {
        return nullptr;
    }
}

void ChunkDownload::getStats(Stats &s)
{
    s.chunk_index = chunk->getIndex();
    s.current_peer_id = getPieceDownloaderName();
    s.download_speed = getDownloadSpeed();
    s.num_downloaders = getNumDownloaders();
    s.pieces_downloaded = num_downloaded;
    s.total_pieces = num;
}

bool ChunkDownload::isChoked() const
{
    QList<PieceDownloader *>::const_iterator i = pdown.begin();
    while (i != pdown.end()) {
        const PieceDownloader *pd = *i;
        // if there is one which isn't choked
        if (!pd->isChoked())
            return false;
        ++i;
    }
    return true;
}

void ChunkDownload::updateHash()
{
    // update the hash until where we can
    Uint32 nn = num_pieces_in_hash;
    while (nn < num && pieces.get(nn))
        nn++;

    for (Uint32 i = num_pieces_in_hash; i < nn; i++) {
        PieceData::Ptr piece = piece_data[i];
        Uint32 len = i == num - 1 ? last_size : MAX_PIECE_LEN;
        if (!piece)
            piece = chunk->getPiece(i * MAX_PIECE_LEN, len, true);

        if (piece && piece->ok()) {
            piece->updateHash(hash_gen);
            chunk->savePiece(piece);
        }
    }
    num_pieces_in_hash = nn;
}

}
