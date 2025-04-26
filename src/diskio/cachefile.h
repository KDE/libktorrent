/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTCACHEFILE_H
#define BTCACHEFILE_H

#include <QFile>
#include <QMap>
#include <QRecursiveMutex>
#include <QSharedPointer>
#include <util/constants.h>

namespace bt
{
class PreallocationThread;

/**
 * Interface which classes must implement to be able to map something from a CacheFile
 * It will also be used to notify when things get unmapped or remapped
 */
class MMappeable
{
public:
    virtual ~MMappeable()
    {
    }

    /**
     * When a CacheFile is closed, this will be called on all existing mappings.
     */
    virtual void unmapped() = 0;
};

/**
    @author Joris Guisson <joris.guisson@gmail.com>

    Used by Single and MultiFileCache to write to disk.
*/
class CacheFile : public QObject
{
    Q_OBJECT
public:
    CacheFile();
    ~CacheFile() override;

    enum Mode {
        READ,
        WRITE,
        RW,
    };

    /**
     * Open the file.
     * @param path Path of the file
     * @param size Max size of the file
     * @throw Error when something goes wrong
     */
    void open(const QString &path, Uint64 size);

    /// Change the path of the file
    void changePath(const QString &npath);

    /**
     * Map a part of the file into memory, will expand the file
     * if it is to small, but will not go past the limit set in open.
     * @param thing The thing that wishes to map the mmapping
     * @param off Offset into the file
     * @param size Size of the region to map
     * @param mode How the region will be mapped
     * @return A ptr to the mmaped region, or 0 if something goes wrong
     */
    void *map(MMappeable *thing, Uint64 off, Uint32 size, Mode mode);

    /**
     * Unmap a previously mapped region.
     * @param ptr Ptr to the region
     */
    void unmap(void *ptr);

    /**
     * Close the file, everything will be unmapped.
     */
    void close();

    /**
     * Read from the file.
     * @param buf Buffer to store data
     * @param size Size to read
     * @param off Offset to read from in file
     */
    void read(Uint8 *buf, Uint32 size, Uint64 off);

    /**
     * Write to the file.
     * @param buf Buffer to write
     * @param size Size to read
     * @param off Offset to read from in file
     */
    void write(const Uint8 *buf, Uint32 size, Uint64 off);

    /**
     * Preallocate disk space
     */
    void preallocate(PreallocationThread *prealloc);

    /// Get the number of bytes this cache file is taking up
    Uint64 diskUsage();

    typedef QSharedPointer<CacheFile> Ptr;

private:
    void growFile(Uint64 to_write);
    void closeTemporary();
    void openFile(Mode mode);
    void unmapAll();
    bool allocateBytes(bt::Uint64 off, bt::Uint64 size);

private Q_SLOTS:
    void aboutToClose();

private:
    QFile *fptr;
    bool read_only;
    Uint64 max_size, file_size;
    QString path;
    struct Entry {
        MMappeable *thing;
        void *ptr;
        Uint32 size;
        Uint64 offset;
        Uint32 diff;
        Mode mode;
    };
    QMap<void *, Entry> mappings; // mappings where offset wasn't a multiple of 4K
    mutable QRecursiveMutex mutex;
    bool manual_close;
};

}

#endif
