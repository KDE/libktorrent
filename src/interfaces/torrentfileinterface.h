/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTTORRENTFILEINTERFACE_H
#define BTTORRENTFILEINTERFACE_H

#include <ktorrent_export.h>
#include <qobject.h>
#include <qstring.h>
#include <util/constants.h>

class QTextCodec;

namespace bt
{
/**
 * @author Joris Guisson
 * @brief Interface for a file in a multifile torrent
 *
 * This class is the interface for a file in a multifile torrent.
 */
class KTORRENT_EXPORT TorrentFileInterface : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor, set the path and size.
     * @param index The index of the file in the torrent
     * @param path The path
     * @param size The size
     */
    TorrentFileInterface(Uint32 index, const QString &path, Uint64 size);
    ~TorrentFileInterface() override;

    enum FileType {
        UNKNOWN,
        AUDIO,
        VIDEO,
        NORMAL,
    };

    /// Get the index of the file
    Uint32 getIndex() const
    {
        return index;
    }

    /// Get the path of the file
    QString getPath() const
    {
        return path;
    }

    /// Get the path of a file on disk
    QString getPathOnDisk() const
    {
        return path_on_disk;
    }

    /// Get the mount point of the file on disk
    QString getMountPoint() const;

    /// Set the mount point
    void setMountPoint(const QString &path)
    {
        mount_point = path;
    }

    /**
     * Set the actual path of the file on disk.
     * @param p The path
     */
    void setPathOnDisk(const QString &p)
    {
        path_on_disk = p;
    }

    /// Get user modified path (if isn't changed, the normal path is returned)
    QString getUserModifiedPath() const
    {
        return user_modified_path.isEmpty() ? path : user_modified_path;
    }

    /// Set the user modified path
    void setUserModifiedPath(const QString &p)
    {
        user_modified_path = p;
    }

    /// Get the size of the file
    Uint64 getSize() const
    {
        return size;
    }

    /// Get the index of the first chunk in which this file lies
    Uint32 getFirstChunk() const
    {
        return first_chunk;
    }

    /// Get the last chunk of the file
    Uint32 getLastChunk() const
    {
        return last_chunk;
    }

    /// Get the offset at which the file starts in the first chunk
    Uint64 getFirstChunkOffset() const
    {
        return first_chunk_off;
    }

    /// Get how many bytes the files takes up of the last chunk
    Uint64 getLastChunkSize() const
    {
        return last_chunk_size;
    }

    /// See if the TorrentFile is null.
    bool isNull() const
    {
        return path.isNull();
    }

    /// Set whether we have to not download this file
    virtual void setDoNotDownload(bool dnd) = 0;

    /// Whether or not we have to not download this file
    virtual bool doNotDownload() const = 0;

    /// Checks if this file is multimedial
    virtual bool isMultimedia() const = 0;

    /// Gets the current priority of the torrent
    virtual Priority getPriority() const
    {
        return priority;
    }

    /// Sets the priority of the torrent
    virtual void setPriority(Priority newpriority = NORMAL_PRIORITY) = 0;

    /// Wheather to emit signal when dl status changes or not.
    virtual void setEmitDownloadStatusChanged(bool show) = 0;

    /// Emits signal dlStatusChanged. Use it only with FileSelectDialog!
    virtual void emitDownloadStatusChanged() = 0;

    /// Did this file exist before the torrent was loaded by KT
    bool isPreExistingFile() const
    {
        return preexisting;
    }

    /// Set whether this file is preexisting
    void setPreExisting(bool pe)
    {
        preexisting = pe;
    }

    /// Get the % of the file which is downloaded
    float getDownloadPercentage() const;

    /// See if preview is available
    bool isPreviewAvailable() const
    {
        return preview;
    }

    /// Set the unencoded path
    void setUnencodedPath(const QList<QByteArray> up);

    /// Change the text codec
    void changeTextCodec(QTextCodec *codec);

    /// Is this a video
    bool isVideo() const
    {
        return filetype == VIDEO;
    }

    /// Is this an audio file
    bool isAudio() const
    {
        return filetype == AUDIO;
    }

protected:
    Uint32 index;
    Uint32 first_chunk;
    Uint32 last_chunk;
    Uint32 num_chunks_downloaded;
    Uint64 size;
    Uint64 first_chunk_off;
    Uint64 last_chunk_size;
    bool preexisting;
    bool emit_status_changed;
    bool preview;
    mutable FileType filetype;
    Priority priority;
    QString path;
    QString path_on_disk;
    QString user_modified_path;
    mutable QString mount_point;
    QList<QByteArray> unencoded_path;
};

}

#endif
