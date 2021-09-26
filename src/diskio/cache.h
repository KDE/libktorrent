/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCACHE_H
#define BTCACHE_H

#include <QMultiMap>
#include <QSet>
#include <QString>
#include <diskio/piecedata.h>
#include <ktorrent_export.h>
#include <torrent/torrent.h>
#include <util/constants.h>

class QStringList;

namespace bt
{
class TorrentFile;
class Chunk;
class PreallocationThread;
class TorrentFileInterface;
class Job;

/**
 * @author Joris Guisson
 * @brief Manages the temporary data
 *
 * Interface for a class which manages downloaded data.
 * Subclasses should implement the load and save methods.
 */
class KTORRENT_EXPORT Cache
{
public:
    Cache(Torrent &tor, const QString &tmpdir, const QString &datadir);
    virtual ~Cache();

    /**
     * Load the file map of a torrent.
     * If it doesn't exist, it needs to be created.
     */
    virtual void loadFileMap() = 0;

    /**
     * Save the file map of a torrent
     */
    virtual void saveFileMap() = 0;

    /// Get the datadir
    QString getDataDir() const
    {
        return datadir;
    }

    /**
     * Get the actual output path.
     * @return The output path
     */
    virtual QString getOutputPath() const = 0;

    /**
     * Changes the tmp dir. All data files should already been moved.
     * This just modifies the tmpdir variable.
     * @param ndir The new tmpdir
     */
    virtual void changeTmpDir(const QString &ndir);

    /**
     * Changes output path. All data files should already been moved.
     * This just modifies the datadir variable.
     * @param outputpath New output path
     */
    virtual void changeOutputPath(const QString &outputpath) = 0;

    /**
     * Move the data files to a new directory.
     * @param ndir The directory
     * @return The job doing the move
     */
    virtual Job *moveDataFiles(const QString &ndir) = 0;

    /**
     * A move of a bunch of data files has finished
     * @param job The job doing the move
     */
    virtual void moveDataFilesFinished(Job *job) = 0;

    /**
     * Load a piece into memory. If something goes wrong,
     * an Error should be thrown.
     * @param c The Chunk
     * @param off The offset of the piece
     * @param length The length of the piece
     * @return Pointer to the data
     */
    virtual PieceData::Ptr loadPiece(Chunk *c, Uint32 off, Uint32 length) = 0;

    /**
     * Prepare a piece for writing. If something goes wrong,
     * an Error should be thrown.
     * @param c The Chunk
     * @param off The offset of the piece
     * @param length The length of the piece
     * @return Pointer to the data
     */
    virtual PieceData::Ptr preparePiece(Chunk *c, Uint32 off, Uint32 length) = 0;

    /**
     * Save a piece to disk, will only actually save in buffered mode
     * @param piece The piece
     */
    virtual void savePiece(PieceData::Ptr piece) = 0;

    /**
     * Create all the data files to store the data.
     */
    virtual void create() = 0;

    /**
     * Close the cache file(s).
     */
    virtual void close() = 0;

    /**
     * Open the cache file(s)
     */
    virtual void open() = 0;

    /// Does nothing, can be overridden to be alerted of download status changes of a TorrentFile
    virtual void downloadStatusChanged(TorrentFile *, bool){};

    /**
     * Prepare disksapce preallocation
     * @param prealloc The thread going to do the preallocation
     */
    virtual void preparePreallocation(PreallocationThread *prealloc) = 0;

    /// See if the download has existing files
    bool hasExistingFiles() const
    {
        return preexisting_files;
    }

    /**
     * Test all files and see if they are not missing.
     * If so put them in a list
     */
    virtual bool hasMissingFiles(QStringList &sl) = 0;

    /**
     * Delete all data files, in case of multi file torrents
     * empty directories should also be deleted.
     * @return The job doing the delete
     */
    virtual Job *deleteDataFiles() = 0;

    /**
     * Move some files to a new location
     * @param files Map of files to move and their new location
     * @return Job The job doing the move
     */
    virtual Job *moveDataFiles(const QMap<TorrentFileInterface *, QString> &files);

    /**
     * The job doing moveDataFiles (with the map parameter) has finished
     * @param files The files map with all the moves
     * @param job The job doing the move
     */
    virtual void moveDataFilesFinished(const QMap<TorrentFileInterface *, QString> &files, Job *job);

    /**
     * See if we are allowed to use mmap, when loading chunks.
     * This will return false if we are close to system limits.
     */
    static bool mappedModeAllowed();

    /**
     * Get the number of bytes all the files of this torrent are currently using on disk.
     * */
    virtual Uint64 diskUsage() = 0;

    /**
     * Determine the mount points of all the files in this torrent
     * @return bool True if we can, false if not
     **/
    virtual bool getMountPoints(QSet<QString> &mps) = 0;

    /**
     * Enable or disable diskspace preallocation
     * @param on
     */
    static void setPreallocationEnabled(bool on)
    {
        preallocate_files = on;
    }

    /**
     * Check if diskspace preallocation is enabled
     * @return true if it is
     */
    static bool preallocationEnabled()
    {
        return preallocate_files;
    }

    /**
     * Enable or disable full diskspace preallocation
     * @param on
     */
    static void setPreallocateFully(bool on)
    {
        preallocate_fully = on;
    }

    /**
     * Check if full diskspace preallocation is enabled.
     * @return true if it is
     */
    static bool preallocateFully()
    {
        return preallocate_fully;
    }

    /**
     * Check memory usage and free all PieceData objects which are no longer needed.
     */
    void checkMemoryUsage();

    /**
     * Clear all pieces of a chunk
     * @param c The chunk
     * */
    void clearPieces(Chunk *c);

    /**
     * Load the mount points of this torrent
     **/
    void loadMountPoints();

    /// Is the storage mounted ?
    bool isStorageMounted(QStringList &missing);

protected:
    PieceData::Ptr findPiece(Chunk *c, Uint32 off, Uint32 len, bool read_only);
    void insertPiece(Chunk *c, PieceData::Ptr p);
    void clearPieceCache();
    void cleanupPieceCache();
    void saveMountPoints(const QSet<QString> &mp);

protected:
    Torrent &tor;
    QString tmpdir;
    QString datadir;
    bool preexisting_files;
    Uint32 mmap_failures;

    typedef QMultiMap<Chunk *, PieceData::Ptr> PieceCache;
    PieceCache piece_cache;

    QSet<QString> mount_points;

private:
    static bool preallocate_files;
    static bool preallocate_fully;
};

}

#endif
