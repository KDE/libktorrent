/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTDOWNLOADER_H
#define BTDOWNLOADER_H

#include "download/chunkdownload.h"
#include "download/webseed.h"
#include "peer/peermanager.h"
#include <QObject>
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/ptrmap.h>

#include <memory>

class QUrl;

namespace bt
{
class BitSet;
class Torrent;
class ChunkManager;
class PeerManager;
class Peer;
class Chunk;
class Piece;
class ChunkSelectorInterface;
class PieceDownloader;
class MonitorInterface;
class WebSeedChunkDownload;

typedef PtrMap<Uint32, ChunkDownload>::iterator CurChunkItr;
typedef PtrMap<Uint32, ChunkDownload>::const_iterator CurChunkCItr;

#define CURRENT_CHUNK_MAGIC 0xABCDEF00

struct CurrentChunksHeader {
    Uint32 magic; // CURRENT_CHUNK_MAGIC
    Uint32 major;
    Uint32 minor;
    Uint32 num_chunks;
};

/*!
 * \author Joris Guisson
 * \brief Manages the downloading
 *
 * This class manages the downloading of the file. It should
 * regurarly be updated.
 */
class KTORRENT_EXPORT Downloader : public QObject, public PieceHandler
{
    Q_OBJECT

public:
    /*!
     * Constructor.
     * \param tor The Torrent
     * \param pman The PeerManager
     * \param cman The ChunkManager
     */
    Downloader(Torrent &tor, PeerManager &pman, ChunkManager &cman);
    ~Downloader() override;

    /*!
     * Set the group ID's of the webseed (for speed limits)
     * \param up Upload group id
     * \param down Download group id
     */
    void setGroupIDs(Uint32 up, Uint32 down);

    //! Get the number of webseeds
    Uint32 getNumWebSeeds() const
    {
        return webseeds.count();
    }

    //! Get a webseed
    const WebSeed *getWebSeed(Uint32 i) const
    {
        return i < (Uint32)webseeds.count() ? webseeds[i] : nullptr;
    }

    //! Get a webseed
    WebSeed *getWebSeed(Uint32 i)
    {
        return i < (Uint32)webseeds.count() ? webseeds[i] : nullptr;
    }

    //! Add a webseed
    WebSeed *addWebSeed(const QUrl &url);

    //! Remove a webseed
    bool removeWebSeed(const QUrl &url);

    //! Remove all webseeds
    void removeAllWebSeeds();

    //! Save the user created webseeds
    void saveWebSeeds(const QString &file);

    //! Add the user created webseeds
    void loadWebSeeds(const QString &file);

    //! Get the number of bytes we have downloaded
    Uint64 bytesDownloaded() const
    {
        return bytes_downloaded + curr_chunks_downloaded;
    }

    //! Get the current dowload rate
    Uint32 downloadRate() const;

    //! Get the number of chunks we are dowloading
    Uint32 numActiveDownloads() const
    {
        return current_chunks.count() + active_webseed_downloads;
    }

    //! See if the download is finished.
    bool isFinished() const;

    /*!
     * Clear all downloads. Deletes all active downloads.
     */
    void clearDownloads();

    /*!
     * Pause the download
     */
    void pause();

    CurChunkCItr beginDownloads() const
    {
        return current_chunks.begin();
    }
    CurChunkCItr endDownloads() const
    {
        return current_chunks.end();
    }

    /*!
     * Get a download for a chunk
     * \param chunk The chunk
     * \return The ChunkDownload, or 0 if no download is found
     */
    ChunkDownload *download(Uint32 chunk);

    /*!
     * Get a download for a chunk (const version)
     * \param chunk The chunk
     * \return The ChunkDownload, or 0 if no download is found
     */
    const ChunkDownload *download(Uint32 chunk) const;

    /*!
     * See if we are downloading a Chunk
     * \param chunk ID of Chunk
     * \return true if we are, false if not
     */
    bool downloading(Uint32 chunk) const;

    /*!
     * Can we download a chunk from a webseed.
     * \param chunk ID of Chunk
     * \return true if we can
     */
    bool canDownloadFromWebSeed(Uint32 chunk) const;

    /*!
     * Get the number of downloaders assigned to a chunk
     * \param chunk ID of Chunk
     * \return the number of downloaders for that chunk
     */
    Uint32 numDownloadersForChunk(Uint32 chunk) const;

    //! Are we in endgame mode
    bool endgameMode() const;

    /*!
     * Save the current downloads.
     * \param file The file to save to
     */
    void saveDownloads(const QString &file);

    /*!
     * Load the current downloads.
     * \param file The file to load from
     */
    void loadDownloads(const QString &file);

    /*!
     * Get the number of bytes already downloaded in the current_chunks file.
     * \param file The path of the current_chunks file
     * \return The bytes already downloading
     */
    Uint32 getDownloadedBytesOfCurrentChunksFile(const QString &file);

    /*!
     * A corrupted chunk has been detected, make sure we redownload it.
     * \param chunk The chunk
     */
    void corrupted(Uint32 chunk);

    //! Set the ChunkSelector to use (nullptr resets to the default ChunkSelector)
    void setChunkSelector(std::unique_ptr<ChunkSelectorInterface> csel);

    /*!
     * We got a new connection.
     * \param peer The PieceDownloader
     */
    void addPieceDownloader(PieceDownloader *peer);

    /*!
     * Remove a piece downloader.
     * \param peer The PieceDownloader
     */
    void removePieceDownloader(PieceDownloader *peer);

    //! Enable or disable the use of webseeds
    static void setUseWebSeeds(bool on);
public Q_SLOTS:
    /*!
     * Update the downloader.
     */
    void update();

    /*!
     * Set the TorrentMonitor.
     * \param tmo
     */
    void setMonitor(bt::MonitorInterface *tmo);

    /*!
     * Data has been checked, and these chunks are OK.
     * \param ok_chunks The ok_chunks
     * \param from First chunk of the check
     * \param to Last chunk of the check
     */
    void dataChecked(const bt::BitSet &ok_chunks, bt::Uint32 from, bt::Uint32 to);

    /*!
     * Recalculate the number of bytes downloaded.
     */
    void recalcDownloaded();

private Q_SLOTS:
    void pieceReceived(const bt::Piece &p) override;
    bool finished(bt::ChunkDownload *c);

public:
    /*!
     * Kill all ChunkDownload's which have been excluded.
     * \param from First chunk of range
     * \param to Last chunk of range
     */
    void onExcluded(Uint32 from, Uint32 to);

    /*!
     * Make sure chunk selector is back OK, when chunks are included back again.
     * \param from First chunk
     * \param to Last chunk
     */
    void onIncluded(Uint32 from, Uint32 to);

private:
    /*!
     * A WebSeed has finished a Chunk
     * \param c The chunk
     */
    void onChunkReady(Chunk *c);

    void chunkDownloadStarted(WebSeedChunkDownload *cd, Uint32 chunk);
    void chunkDownloadFinished(WebSeedChunkDownload *cd, Uint32 chunk);

Q_SIGNALS:
    /*!
     * An error occurred while we we're writing or reading from disk.
     * \param msg Message
     */
    void ioError(const QString &msg);

    /*!
     * Emitted when a chunk has been downloaded.
     * \param chunk The chunk
     */
    void chunkDownloaded(Uint32 chunk);

private:
    bool downloadFrom(PieceDownloader *pd);
    void downloadFrom(WebSeed *ws);
    void normalUpdate();
    bool findDownloadForPD(PieceDownloader *pd);
    ChunkDownload *selectCD(PieceDownloader *pd, Uint32 num);
    ChunkDownload *selectWorst(PieceDownloader *pd);

private:
    Torrent &tor;
    PeerManager &pman;
    ChunkManager &cman;
    Uint64 bytes_downloaded;
    Uint64 curr_chunks_downloaded;
    Uint64 unnecessary_data;
    PtrMap<Uint32, ChunkDownload> current_chunks;
    QList<PieceDownloader *> piece_downloaders;
    MonitorInterface *tmon;
    std::unique_ptr<ChunkSelectorInterface> chunk_selector;
    QList<WebSeed *> webseeds;
    PtrMap<Uint32, WebSeed> webseeds_chunks;
    Uint32 active_webseed_downloads;
    bool webseeds_on;
    Uint32 webseed_range_size;
    bool webseed_endgame_mode;

    static bool use_webseeds;
};

}

#endif
