/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_TORRENTFILESTREAM_H
#define BT_TORRENTFILESTREAM_H

#include <QIODevice>
#include <QSharedPointer>
#include <QWeakPointer>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
class ChunkManager;
class TorrentControl;
class TorrentInterface;
class BitSet;

/**
    QIODevice which streams a file of a torrent or the whole torrent (for single file torrents)
    This object should not be manually constructed.
*/
class KTORRENT_EXPORT TorrentFileStream : public QIODevice
{
public:
    TorrentFileStream(TorrentControl *tc, ChunkManager *cman, bool streaming_mode, QObject *parent);
    TorrentFileStream(TorrentControl *tc, Uint32 file_index, ChunkManager *cman, bool streaming_mode, QObject *parent);
    ~TorrentFileStream() override;

    /// Open the device (only readonly access will be allowed)
    bool open(QIODevice::OpenMode mode) override;

    /// Close the device
    void close() override;

    /// Get the current stream position
    qint64 pos() const override;

    /// Get the total size
    qint64 size() const override;

    /// Seek, will fail if attempting to seek to a point which is not downloaded yet
    bool seek(qint64 pos) override;

    /// Are we at the end of the file
    bool atEnd() const override;

    /// Reset the stream
    bool reset() override;

    /// How many bytes are there available
    qint64 bytesAvailable() const override;

    /// The stream is not sequential
    bool isSequential() const override
    {
        return false;
    }

    /// Get the path of the file
    QString path() const;

    /// Get a BitSet of all the chunks of this TorrentFileStream
    const BitSet &chunksBitSet() const;

    /// Get the current chunk relative to the first chunk of the file
    Uint32 currentChunk() const;

    typedef QSharedPointer<TorrentFileStream> Ptr;
    typedef QWeakPointer<TorrentFileStream> WPtr;

protected:
    qint64 writeData(const char *data, qint64 len) override;
    qint64 readData(char *data, qint64 maxlen) override;
    void emitReadChannelFinished();

private:
    void chunkDownloaded(bt::TorrentInterface *tc, bt::Uint32 chunk);

    class Private;
    Private *d;
};

}

#endif // BT_TORRENTFILESTREAM_H
