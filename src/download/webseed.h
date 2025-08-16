/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTWEBSEED_H
#define BTWEBSEED_H

#include <QTimer>
#include <QUrl>
#include <diskio/piecedata.h>
#include <interfaces/chunkdownloadinterface.h>
#include <interfaces/webseedinterface.h>
#include <ktorrent_export.h>
#include <peer/connectionlimit.h>
#include <util/constants.h>

namespace bt
{
class Torrent;
class HttpConnection;
class ChunkManager;
class Chunk;
class WebSeedChunkDownload;

/*!
    \author Joris Guisson
    Class which handles downloading from a webseed
*/
class KTORRENT_EXPORT WebSeed : public QObject, public WebSeedInterface
{
    Q_OBJECT
public:
    WebSeed(const QUrl &url, bool user, const Torrent &tor, ChunkManager &cman);
    ~WebSeed() override;

    //! Is this webseed busy ?
    bool busy() const;

    //! Check if a chunk lies in the current range we are downloading
    bool inCurrentRange(Uint32 chunk) const
    {
        return chunk >= first_chunk && chunk <= last_chunk;
    }

    /*!
     * Download a range of chunks
     * \param first The first chunk
     * \param last The last chunk
     */
    void download(Uint32 first, Uint32 last);

    /*!
     * A range has been excluded, if we are fully
     * downloading in this range, reset.
     * \param from Start of range
     * \param to End of range
     */
    void onExcluded(Uint32 from, Uint32 to);

    /*!
     * Check if the connection has received some data and handle it.
     * \return The number of bytes downloaded
     */
    Uint32 update();

    /*!
     * A chunk has been downloaded.
     * \param chunk The chunk
     */
    void chunkDownloaded(Uint32 chunk);

    /*!
     * Cancel the current download and kill the connection
     */
    void cancel();

    //! Get the current download rate
    Uint32 getDownloadRate() const override;

    /*!
     * Set the group ID's of the http connection (for speed limits)
     * \param up Upload group id
     * \param down Download group id
     */
    void setGroupIDs(Uint32 up, Uint32 down);

    /*!
     * Set the proxy to use for all WebSeeds
     * \param host Hostname or IP address of the proxy
     * \param port Port number of the proxy
     */
    static void setProxy(const QString &host, bt::Uint16 port);

    /*!
     * Whether or not to enable or disable the use of a proxy.
     * When the proxy is disabled, we will use the KDE proxy settings.
     * \param on On or not
     */
    static void setProxyEnabled(bool on);

    //! Get the current webseed download
    WebSeedChunkDownload *currentChunkDownload()
    {
        return current;
    }

    void setEnabled(bool on) override;

    //! Disable the webseed
    void disable(const QString &reason);

    //! Get the number of failed attempts
    Uint32 failedAttempts() const
    {
        return num_failures;
    }

public Q_SLOTS:
    /*!
     * Reset the webseed (kills the connection)
     */
    void reset();

Q_SIGNALS:
    /*!
     * Emitted when a chunk is downloaded
     * \param c The chunk
     */
    void chunkReady(bt::Chunk *c);

    /*!
     * Emitted when a range has been fully downloaded
     */
    void finished();

    /*!
     * A ChunkDownload was started
     * \param cd The ChunkDownloadInterface
     * \param chunk The chunk which is being started
     */
    void chunkDownloadStarted(bt::WebSeedChunkDownload *cd, Uint32 chunk);

    /*!
     * A ChunkDownload was finished
     * \param cd The ChunkDownloadInterface
     * \param chunk The chunk which is being stopped
     */
    void chunkDownloadFinished(bt::WebSeedChunkDownload *cd, Uint32 chunk);

private Q_SLOTS:
    void redirected(const QUrl &to_url);

private:
    struct Range {
        Uint32 file;
        Uint64 off;
        Uint64 len;
    };

    class AutoDisabled
    {
    }; // Exception

    void fillRangeList(Uint32 chunk);
    void handleData(const QByteArray &data);
    void chunkStarted(Uint32 chunk);
    void chunkStopped();
    void connectToServer();
    void continueCurChunk();
    void readData();
    void retryLater();

private:
    const Torrent &tor;
    ChunkManager &cman;
    HttpConnection *conn = nullptr;
    QList<QByteArray> chunks;
    Uint32 first_chunk;
    Uint32 last_chunk;
    Uint32 cur_chunk = -1;
    Uint32 bytes_of_cur_chunk;
    Uint32 num_failures = 0;
    Uint32 downloaded = 0;
    WebSeedChunkDownload *current = nullptr;
    Uint32 up_gid = 0;
    Uint32 down_gid = 0;
    QList<Range> range_queue;
    QUrl redirected_url;
    PieceData::Ptr cur_piece;
    QTimer retry_timer;
    ConnectionLimit::Token::Ptr token;

    static QString proxy_host;
    static Uint16 proxy_port;
    static bool proxy_enabled;
};

class WebSeedChunkDownload : public ChunkDownloadInterface
{
public:
    WebSeedChunkDownload(WebSeed *ws, const QString &url, Uint32 index, Uint32 total);
    ~WebSeedChunkDownload() override;

    void getStats(Stats &s) override;

    WebSeed *ws;
    QString url;
    Uint32 chunk;
    Uint32 total_pieces;
    Uint32 pieces_downloaded;
};

}

#endif
