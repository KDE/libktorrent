/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "cachefile.h"

#include <config-ktorrent.h>

#include <KFileItem>
#include <KLocalizedString>

#include <QFile>
#include <QStorageInfo>

#include "cache.h"
#include "preallocationthread.h"
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <util/array.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/log.h>

#ifndef Q_OS_WIN
#include <sys/mman.h>
#endif

// Not all systems have an O_LARGEFILE - Solaris depending
// on command-line defines, FreeBSD never - so in those cases,
// make it a zero bitmask. As long as it's only OR'ed into
// open(2) flags, that's fine.
//
#ifndef O_LARGEFILE
#define O_LARGEFILE (0)
#endif

using namespace Qt::Literals::StringLiterals;

namespace bt
{
#ifndef Q_OS_WIN
const Uint64 CacheFile::page_size = sysconf(_SC_PAGESIZE);
#endif

CacheFile::CacheFile()
    : fptr(nullptr)
    , max_size(0)
    , file_size(0)
    , mutex()
{
    read_only = false;
    manual_close = false;
}

CacheFile::~CacheFile()
{
    if (fptr)
        close();
}

void CacheFile::changePath(const QString &npath)
{
    path = npath;
}

void CacheFile::openFile(Mode mode)
{
    // by default always try read write
    fptr = new QFile(path);
    connect(fptr, &QFile::aboutToClose, this, &CacheFile::aboutToClose);
    bool ok = false;
    if (!(ok = fptr->open(QIODevice::ReadWrite))) {
        // in case RDWR fails, try readonly if possible
        if (mode == READ && (ok = fptr->open(QIODevice::ReadOnly)))
            read_only = true;
    }

    if (!ok) {
        delete fptr;
        fptr = nullptr;
        const int err = errno;
        throw Error(i18n("Cannot open %1: %2", path, QString::fromUtf8(strerror(err))));
    }

    file_size = fptr->size();
}

void CacheFile::open(const QString &path, Uint64 size)
{
    QMutexLocker lock(&mutex);
    // only set the path and the max size, we only open the file when it is needed
    this->path = path;
    max_size = size;
}

void *CacheFile::map(MMappeable *thing, Uint64 off, Uint32 size, Mode mode)
{
    QMutexLocker lock(&mutex);
    // reopen the file if necessary
    if (!fptr) {
        //  Out(SYS_DIO|LOG_DEBUG) << "Reopening " << path << endl;
        QStorageInfo mount(path); // ntfs cannot handle mmap properly
        if (!OpenFileAllowed() || mount.fileSystemType() == "fuseblk" || mount.fileSystemType().startsWith("ntfs"))
            return nullptr; // Running out of file descriptors, force buffered mode
        openFile(mode);
    }

    if (read_only && mode != READ) {
        throw Error(i18n("Cannot open %1 for writing: readonly filesystem", path));
    }

    if (off + size > max_size) {
        Out(SYS_DIO | LOG_DEBUG) << "Warning : writing past the end of " << path << endl;
        Out(SYS_DIO | LOG_DEBUG) << (off + size) << " " << max_size << endl;
        throw Error(i18n("Attempting to write beyond the maximum size of %1", path));
    }

    /*
    if (!read_only && (mode == WRITE || mode == RW) && !allocateBytes(off,size))
        throw Error(i18n("Not enough free disk space for %1",path));
    */

    if (off + size > file_size) {
        Uint64 to_write = (off + size) - file_size;
        //  Out(SYS_DIO|LOG_DEBUG) << "Growing file with " << to_write << " bytes" << endl;
        growFile(to_write);
    }

#ifndef Q_OS_WIN
    int mmap_flag = 0;
    switch (mode) {
    case READ:
        mmap_flag = PROT_READ;
        break;
    case WRITE:
        mmap_flag = PROT_WRITE;
        break;
    case RW:
        mmap_flag = PROT_READ | PROT_WRITE;
        break;
    }

    int fd = fptr->handle();
    // off may not be a multiple of the page_size
    // so we play around a bit
    Uint64 diff = (off % page_size);
    Uint64 noff = off - diff;
    //  Out(SYS_DIO|LOG_DEBUG) << "Offsetted mmap : " << diff << endl;
#ifdef HAVE_MMAP64
    char *ptr = (char *)mmap64(nullptr, size + diff, mmap_flag, MAP_SHARED, fd, noff);
#else
    char *ptr = (char *)mmap(0, size + diff, mmap_flag, MAP_SHARED, fd, noff);
#endif
    if (ptr == MAP_FAILED) {
        const int err = errno;
        Out(SYS_DIO | LOG_DEBUG) << "mmap failed : " << QString::fromUtf8(strerror(err)) << endl;
        return nullptr;
    } else {
        CacheFile::Entry e;
        e.thing = thing;
        e.offset = off;
        e.ptr = ptr;
        e.size = size + diff;
        e.mode = mode;
        mappings.insert((void *)(ptr + diff), e);
        return ptr + diff;
    }
#else // Q_OS_WIN
    char *ptr = (char *)fptr->map(off, size);

    if (!ptr) {
        const int err = errno;
        Out(SYS_DIO | LOG_DEBUG) << "mmap failed3 : " << fptr->handle() << " " << QString::fromUtf8(strerror(err)) << endl;
        Out(SYS_DIO | LOG_DEBUG) << off << " " << size << endl;
        return nullptr;
    } else {
        CacheFile::Entry e;
        e.thing = thing;
        e.offset = off;
        e.ptr = ptr;
        e.size = size;
        e.mode = mode;
        mappings.insert(ptr, e);
        return ptr;
    }
#endif
}

void CacheFile::growFile(Uint64 to_write)
{
    // reopen the file if necessary
    if (!fptr) {
        //  Out(SYS_DIO|LOG_DEBUG) << "Reopening " << path << endl;
        openFile(RW);
    }

    if (read_only)
        throw Error(i18n("Cannot open %1 for writing: readonly filesystem", path));

    if (file_size + to_write > max_size) {
        Out(SYS_DIO | LOG_DEBUG) << "Warning : writing past the end of " << path << endl;
        Out(SYS_DIO | LOG_DEBUG) << (file_size + to_write) << " " << max_size << endl;
        throw Error(i18n("Cannot expand file %1: attempting to grow the file beyond the maximum size", path));
    }

    if (!fptr->resize(file_size + to_write))
        throw Error(i18n("Cannot expand file %1: %2", path, fptr->errorString()));

    file_size = fptr->size();
}

void CacheFile::unmap(void *ptr)
{
    int ret = 0;
    QMutexLocker lock(&mutex);
#ifdef Q_OS_WIN
    if (!fptr)
        return;

    const auto it = mappings.constFind(ptr);
    if (it != mappings.constEnd()) {
        const CacheFile::Entry &e = it.value();
        if (!fptr->unmap((uchar *)e.ptr))
            Out(SYS_DIO | LOG_IMPORTANT) << u"Unmap failed : %1"_s.arg(fptr->errorString()) << endl;

        mappings.erase(it);
        // no mappings, close temporary
        if (mappings.count() == 0)
            closeTemporary();
    }
#else
    const auto it = mappings.constFind(ptr);
    if (it != mappings.constEnd()) {
        const CacheFile::Entry &e = it.value();
#ifdef HAVE_MUNMAP64
        ret = munmap64(e.ptr, e.size);
#else
        ret = munmap(e.ptr, e.size);
#endif
        if (ret < 0) {
            const int err = errno;
            Out(SYS_DIO | LOG_IMPORTANT) << u"Munmap failed with error %1 : %2"_s.arg(err).arg(QString::fromUtf8(strerror(err))) << endl;
        }

        mappings.erase(it);
        // no mappings, close temporary
        if (mappings.count() == 0)
            closeTemporary();
    }
#endif
}

void CacheFile::aboutToClose()
{
    QMutexLocker lock(&mutex);
    if (!fptr)
        return;
    // Out(SYS_DIO|LOG_NOTICE) << "CacheFile " << path << " : about to be closed" << endl;
    unmapAll();
    if (!manual_close) {
        manual_close = true;
        fptr->deleteLater();
        fptr = nullptr;
        manual_close = false;
    }
}

void CacheFile::unmapAll()
{
    auto i = mappings.begin();
    while (i != mappings.end()) {
        int ret = 0;
        CacheFile::Entry &e = i.value();
#ifdef Q_OS_WIN
        fptr->unmap((uchar *)e.ptr);
#else
#ifdef HAVE_MUNMAP64
        ret = munmap64(e.ptr, e.size);
#else
        ret = munmap(e.ptr, e.size);
#endif // HAVE_MUNMAP64
#endif // Q_OS_WIN
        if (ret < 0) {
            const int err = errno;
            Out(SYS_DIO | LOG_IMPORTANT) << u"Munmap failed with error %1 : %2"_s.arg(err).arg(QString::fromUtf8(strerror(err))) << endl;
        }

        MMappeable *thing = e.thing;
        // if it will be reopenend, we will not remove all mappings
        // so that they will be redone on reopening
        i = mappings.erase(i);

        thing->unmapped();
    }
}

void CacheFile::close()
{
    QMutexLocker lock(&mutex);

    if (!fptr)
        return;

    unmapAll();
    manual_close = true;
    fptr->close();
    delete fptr;
    fptr = nullptr;
    manual_close = false;
}

void CacheFile::read(Uint8 *buf, Uint32 size, Uint64 off)
{
    QMutexLocker lock(&mutex);
    bool close_again = false;

    // reopen the file if necessary
    if (!fptr) {
        //  Out(SYS_DIO|LOG_DEBUG) << "Reopening " << path << endl;
        openFile(READ);
        close_again = true;
    }

    if (off >= file_size || off >= max_size) {
        throw Error(i18n("Error: Reading past the end of the file %1", path));
    }

    // jump to right position
    if (!fptr->seek(off))
        throw Error(i18n("Failed to seek file %1: %2", path, fptr->errorString()));

    Uint32 sz = 0;
    if ((sz = fptr->read((char *)buf, size)) != size) {
        if (close_again)
            closeTemporary();

        throw Error(i18n("Error reading from %1", path));
    }

    if (close_again)
        closeTemporary();
}

void CacheFile::write(const Uint8 *buf, Uint32 size, Uint64 off)
{
    QMutexLocker lock(&mutex);
    bool close_again = false;

    // reopen the file if necessary
    if (!fptr) {
        //  Out(SYS_DIO|LOG_DEBUG) << "Reopening " << path << endl;
        openFile(RW);
        close_again = true;
    }

    if (read_only)
        throw Error(i18n("Cannot open %1 for writing: readonly filesystem", path));

    if (off + size > max_size) {
        Out(SYS_DIO | LOG_DEBUG) << "Warning : writing past the end of " << path << endl;
        Out(SYS_DIO | LOG_DEBUG) << (off + size) << " " << max_size << endl;
        throw Error(i18n("Attempting to write beyond the maximum size of %1", path));
    }

    if (file_size < off) {
        // Out(SYS_DIO|LOG_DEBUG) << QString("Writing %1 bytes at %2").arg(size).arg(off) << endl;
        growFile(off - file_size);
    }

    // jump to right position
    if (!fptr->seek(off))
        throw Error(i18n("Failed to seek file %1: %2", path, fptr->errorString()));

    if (fptr->write((const char *)buf, size) != size) {
        throw Error(i18n("Failed to write to file %1: %2", path, fptr->errorString()));
    }

    if (close_again)
        closeTemporary();

    if (off + size > file_size)
        file_size = off + size;
}

void CacheFile::closeTemporary()
{
    if (!fptr || mappings.count() > 0)
        return;

    delete fptr;
    fptr = nullptr;
}

void CacheFile::preallocate(PreallocationThread *prealloc)
{
    QMutexLocker lock(&mutex);

    if (FileSize(path) == max_size) {
        Out(SYS_GEN | LOG_NOTICE) << "File " << path << " already big enough" << endl;
        return;
    }

    Out(SYS_GEN | LOG_NOTICE) << "Preallocating file " << path << " (" << max_size << " bytes)" << endl;
    bool close_again = false;
    if (!fptr) {
        openFile(RW);
        close_again = true;
    }

    int fd = fptr->handle();

    if (read_only) {
        if (close_again)
            closeTemporary();

        throw Error(i18n("Cannot open %1 for writing: readonly filesystem", path));
    }

    try {
        bool res = false;

#ifdef HAVE_XFS_XFS_H
        if (Cache::preallocateFully()) {
            res = XfsPreallocate(fd, max_size);
        }
#endif

        if (!res) {
            bt::TruncateFile(fd, max_size, !Cache::preallocateFully());
        }
    } catch (bt::Error &e) {
        const int err = errno;
        throw Error(i18n("Cannot preallocate diskspace: %1", QString::fromUtf8(strerror(err))));
    }

    file_size = FileSize(fd);
    prealloc->written(file_size);
    Out(SYS_GEN | LOG_DEBUG) << "file_size = " << file_size << endl;
    if (close_again)
        closeTemporary();
}

Uint64 CacheFile::diskUsage()
{
    if (!fptr)
        return DiskUsage(path);
    else
        return DiskUsage(fptr->handle());
}

bool CacheFile::allocateBytes(Uint64 off, Uint64 size)
{
#ifdef HAVE_POSIX_FALLOCATE64
    return posix_fallocate64(fptr->handle(), off, size) != ENOSPC;
#elif HAVE_POSIX_FALLOCATE
    return posix_fallocate(fptr->handle(), off, size) != ENOSPC;
#else
    return true;
#endif
}

}

#include "moc_cachefile.cpp"
