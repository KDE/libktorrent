/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fileops.h"
#include "config-ktorrent.h"

#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef Q_OS_WIN
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <QDir>
#include <QFile>
#include <QSet>
#include <QStringList>

#include <KIO/CopyJob>
#include <KIO/FileCopyJob>
#include <KIO/Job>
#include <KLocalizedString>
#include <Solid/Device>
#include <Solid/StorageAccess>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>

#include "array.h"
#include "error.h"
#include "file.h"
#include "functions.h"
#include "log.h"
#ifdef Q_OS_WIN
#include "win32.h"
#endif

#include "limits.h"

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef HAVE_XFS_XFS_H

#if !defined(HAVE___S64) || !defined(HAVE___U64)
#include <cstdint>
#endif

#ifndef HAVE___U64
typedef uint64_t __u64;
#endif

#ifndef HAVE___S64
typedef int64_t __s64;
#endif

#include <xfs/xfs.h>
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef Q_OS_WIN
#ifdef Q_OS_LINUX
#include <mntent.h>
#endif
#include <sys/statvfs.h>
#endif
#ifdef CopyFile
#undef CopyFile
#endif

using namespace Qt::Literals::StringLiterals;

namespace bt
{
void MakeDir(const QString &dir, bool nothrow)
{
    QDir d(dir);
    if (d.exists())
        return;

    QString n = d.dirName();
    if (!d.cdUp() || !d.mkdir(n)) {
        QString error = i18n("Cannot create directory %1", dir);
        Out(SYS_DIO | LOG_NOTICE) << error << endl;
        if (!nothrow)
            throw Error(error);
    }
}

void MakePath(const QString &dir, bool nothrow)
{
    QStringList sl = dir.split(bt::DirSeparator(), Qt::SkipEmptyParts);
    QString ctmp;
#ifndef Q_OS_WIN
    ctmp += bt::DirSeparator();
#endif

    for (int i = 0; i < sl.count(); i++) {
        ctmp += sl[i];
        if (!bt::Exists(ctmp)) {
            try {
                MakeDir(ctmp, false);
            } catch (...) {
                if (!nothrow)
                    throw;
                return;
            }
        }

        ctmp += bt::DirSeparator();
    }
}

void MakeFilePath(const QString &file, bool nothrow)
{
    QStringList sl = file.split(bt::DirSeparator());
    QString ctmp;
#ifndef Q_OS_WIN
    ctmp += bt::DirSeparator();
#endif

    for (int i = 0; i < sl.count() - 1; i++) {
        ctmp += sl[i];
        if (!bt::Exists(ctmp))
            try {
                MakeDir(ctmp, false);
            } catch (...) {
                if (!nothrow)
                    throw;
                return;
            }

        ctmp += bt::DirSeparator();
    }
}

void SymLink(const QString &link_to, const QString &link_url, bool nothrow)
{
    const QFile symlink_file{link_to};
    const QFile target_file{link_url};
    std::error_code error;
    std::filesystem::create_directory_symlink(target_file.filesystemFileName(), symlink_file.filesystemFileName(), error);
    if (error) {
        const auto error_msg = QString::fromStdString(error.message());
        if (!nothrow)
            throw Error(i18n("Cannot symlink %1 to %2: %3", link_url, link_to, error_msg));
        else
            Out(SYS_DIO | LOG_NOTICE) << QStringLiteral("Error : Cannot symlink %1 to %2: %3").arg(link_url, link_to, error_msg) << endl;
    }
}

void Move(const QString &src, const QString &dst, bool nothrow, bool silent)
{
    //  Out() << "Moving " << src << " -> " << dst << endl;
    KIO::CopyJob *mv = KIO::move(QUrl::fromLocalFile(src), QUrl::fromLocalFile(dst), silent ? KIO::HideProgressInfo | KIO::Overwrite : KIO::DefaultFlags);
    if (!mv->exec()) {
        if (!nothrow)
            throw Error(i18n("Cannot move %1 to %2: %3", src, dst, mv->errorString()));
        else
            Out(SYS_DIO | LOG_NOTICE) << QStringLiteral("Error : Cannot move %1 to %2: %3").arg(src, dst, mv->errorString()) << endl;
    }
}

void CopyFile(const QString &src, const QString &dst, bool nothrow)
{
    KIO::FileCopyJob *copy = KIO::file_copy(QUrl::fromLocalFile(src), QUrl::fromLocalFile(dst));
    if (!copy->exec()) {
        if (!nothrow)
            throw Error(i18n("Cannot copy %1 to %2: %3", src, dst, copy->errorString()));
        else
            Out(SYS_DIO | LOG_NOTICE) << QStringLiteral("Error : Cannot copy %1 to %2: %3").arg(src, dst, copy->errorString()) << endl;
    }
}

void CopyDir(const QString &src, const QString &dst, bool nothrow)
{
    KIO::CopyJob *copy = KIO::copy(QUrl::fromLocalFile(src), QUrl::fromLocalFile(dst));
    if (!copy->exec()) {
        if (!nothrow)
            throw Error(i18n("Cannot copy %1 to %2: %3", src, dst, copy->errorString()));
        else
            Out(SYS_DIO | LOG_NOTICE) << QStringLiteral("Error : Cannot copy %1 to %2: %3").arg(src, dst, copy->errorString()) << endl;
    }
}

bool Exists(const QString &url)
{
    return QFile::exists(url);
}

static bool DelDir(const QString &fn)
{
    QDir d(fn);
    const QStringList subdirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (auto i = subdirs.begin(); i != subdirs.end(); i++) {
        QString entry = *i;

        if (!DelDir(d.absoluteFilePath(entry)))
            return false;
    }

    const QStringList files = d.entryList(QDir::Files | QDir::System | QDir::Hidden);
    for (auto i = files.begin(); i != files.end(); i++) {
        QString file = d.absoluteFilePath(*i);
        QFile fp(file);
        if (!QFileInfo(file).isWritable() && !fp.setPermissions(QFile::ReadUser | QFile::WriteUser))
            return false;

        if (!fp.remove())
            return false;
    }

    if (!d.rmdir(d.absolutePath()))
        return false;

    return true;
}

void Delete(const QString &url, bool nothrow)
{
    bool ok = true;
    // first see if it is a directory
    if (QDir(url).exists()) {
        ok = DelDir(url);
    } else {
        ok = QFile::remove(url);
    }

    if (!ok) {
        QString err = i18n("Cannot delete %1: %2", url, QString::fromUtf8(strerror(errno)));
        if (!nothrow)
            throw Error(err);
        else
            Out(SYS_DIO | LOG_NOTICE) << "Error : " << err << endl;
    }
}

void Touch(const QString &url, bool nothrow)
{
    if (Exists(url))
        return;

    File fptr;
    if (!fptr.open(url, QStringLiteral("wb"))) {
        if (!nothrow)
            throw Error(i18n("Cannot create %1: %2", url, fptr.errorString()));
        else
            Out(SYS_DIO | LOG_NOTICE) << "Error : Cannot create " << url << " : " << fptr.errorString() << endl;
    }
}

Uint64 FileSize(const QString &url)
{
    int ret = 0;
#ifdef HAVE_STAT64
    struct stat64 sb;
    ret = stat64(QFile::encodeName(url).constData(), &sb);
#else
    struct stat sb;
    ret = stat(QFile::encodeName(url).constData(), &sb);
#endif
    if (ret < 0)
        throw Error(i18n("Cannot calculate the filesize of %1: %2", url, QString::fromUtf8(strerror(errno))));

    return (Uint64)sb.st_size;
}

Uint64 FileSize(int fd)
{
    int ret = 0;
#ifdef HAVE_STAT64
    struct stat64 sb;
    ret = fstat64(fd, &sb);
#else
    struct stat sb;
    ret = fstat(fd, &sb);
#endif
    if (ret < 0)
        throw Error(i18n("Cannot calculate the filesize: %1", QString::fromUtf8(strerror(errno))));

    return (Uint64)sb.st_size;
}

#ifdef HAVE_XFS_XFS_H

bool XfsPreallocate(int fd, Uint64 size)
{
    if (!platform_test_xfs_fd(fd)) {
        return false;
    }

    xfs_flock64_t allocopt;
    allocopt.l_whence = 0;
    allocopt.l_start = 0;
    allocopt.l_len = size;

    return (!static_cast<bool>(xfsctl(0, fd, XFS_IOC_RESVSP64, &allocopt)));
}

bool XfsPreallocate(const QString &path, Uint64 size)
{
    int fd = ::open(QFile::encodeName(path).constData(), O_RDWR | O_LARGEFILE);
    if (fd < 0)
        throw Error(i18n("Cannot open %1: %2", path, QString::fromUtf8(strerror(errno))));

    bool ret = XfsPreallocate(fd, size);
    close(fd);
    return ret;
}

#endif

void TruncateFile(int fd, Uint64 size, bool quick)
{
    if (FileSize(fd) == size)
        return;

    if (quick) {
#ifdef HAVE_FTRUNCATE64
        if (ftruncate64(fd, size) == -1)
#elif defined Q_OS_WIN
        if (_chsize_s(fd, size) != 0)
#else
        if (ftruncate(fd, size) == -1)
#endif
            throw Error(i18n("Cannot expand file: %1", QString::fromUtf8(strerror(errno))));
    } else {
#ifdef HAVE_POSIX_FALLOCATE64
        if (posix_fallocate64(fd, 0, size) != 0)
#elif HAVE_POSIX_FALLOCATE
        if (posix_fallocate(fd, 0, size) != 0)
#elif HAVE_FTRUNCATE64
        if (ftruncate64(fd, size) == -1)
#elif defined Q_OS_WIN
        if (_chsize_s(fd, size) != 0)
#else
        if (ftruncate(fd, size) == -1)
#endif
            throw Error(i18n("Cannot expand file: %1", QString::fromUtf8(strerror(errno))));
    }
}

void TruncateFile(const QString &path, Uint64 size)
{
    int fd = ::open(QFile::encodeName(path).constData(), O_RDWR | O_LARGEFILE);
    if (fd < 0)
        throw Error(i18n("Cannot open %1: %2", path, QString::fromUtf8(strerror(errno))));

    try {
        TruncateFile(fd, size, true);
        close(fd);
    } catch (...) {
        close(fd);
        throw;
    }
}

void SeekFile(int fd, Int64 off, int whence)
{
#ifdef HAVE_LSEEK64
    if (lseek64(fd, off, whence) == -1)
#else
    if (lseek(fd, off, whence) == -1)
#endif
        throw Error(i18n("Cannot seek in file: %1", QString::fromUtf8(strerror(errno))));
}

bool FreeDiskSpace(const QString &path, Uint64 &bytes_free)
{
#ifdef HAVE_STATVFS
#ifdef HAVE_STATVFS64
    struct statvfs64 stfs;
    if (statvfs64(QFile::encodeName(path).constData(), &stfs) == 0)
#else
    struct statvfs stfs;
    if (statvfs(QFile::encodeName(path).constData(), &stfs) == 0)
#endif
    {
        if (stfs.f_blocks == 0) // if this is 0, then we are using gvfs
            return false;

        bytes_free = ((Uint64)stfs.f_bavail) * ((Uint64)stfs.f_frsize);
        return true;
    } else {
        Out(SYS_GEN | LOG_DEBUG) << "Error : statvfs for " << path << " failed :  " << QString::fromUtf8(strerror(errno)) << endl;

        return false;
    }
#elif defined(Q_OS_WIN)
#ifdef UNICODE
    LPCWSTR tpath = (LPCWSTR)path.utf16();
#else
    const char *tpath = path.toLocal8Bit();
#endif
    if (GetDiskFreeSpaceEx(tpath, (PULARGE_INTEGER)&bytes_free, NULL, NULL)) {
        return true;
    } else {
        return false;
    }
#else
    return false;
#endif
}

bool FileNameTooLong(const QString &path)
{
    int length = 0;
    const QStringList names = path.split(QLatin1Char('/'));
    for (const QString &s : names) {
        QByteArray encoded = QFile::encodeName(s);
        if (encoded.length() >= NAME_MAX)
            return true;
        length += encoded.length();
    }

    length += path.count(QLatin1Char('/'));
    return length >= PATH_MAX;
}

static QString ShortenName(const QString &name, int extra_number)
{
    QFileInfo fi(name);
    QString ext = fi.suffix();
    QString base = fi.completeBaseName();

    // calculate the fixed length, 1 is for the . between filename and extension
    int fixed_len = 0;
    if (ext.length() > 0)
        fixed_len += QFile::encodeName(ext).length() + 1;
    if (extra_number > 0)
        fixed_len += QFile::encodeName(QString::number(extra_number)).length();

    // if we can't shorten it, give up
    if (fixed_len > NAME_MAX - 4)
        return name;

    do {
        base.chop(1);
    } while (fixed_len + QFile::encodeName(base).length() > NAME_MAX - 4 && base.length() != 0);

    base += "... "_L1; // add ... so that the user knows the name is shortened

    QString ret = base;
    if (extra_number > 0)
        ret += QString::number(extra_number);
    if (ext.length() > 0)
        ret += QLatin1Char('.') + ext;
    return ret;
}

static QString ShortenPath(const QString &path, int extra_number)
{
    int max_len = PATH_MAX;
    QByteArray encoded = QFile::encodeName(path);
    if (encoded.length() < max_len)
        return path;

    QFileInfo fi(path);
    QString ext = fi.suffix();
    QString name = fi.completeBaseName();
    QString fpath = fi.path() + '/'_L1;

    // calculate the fixed length, 1 is for the . between filename and extension
    int fixed_len = QFile::encodeName(fpath).length();
    if (ext.length() > 0)
        fixed_len += QFile::encodeName(ext).length() + 1;
    if (extra_number > 0)
        fixed_len += QFile::encodeName(QString::number(extra_number)).length();

    // if we can't shorten it, give up
    if (fixed_len > max_len - 4)
        return path;

    do {
        name.chop(1);
    } while (fixed_len + QFile::encodeName(name).length() > max_len - 4 && name.length() != 0);

    name += QLatin1String("... "); // add ... so that the user knows the name is shortened

    QString ret = fpath + name;
    if (extra_number > 0)
        ret += QString::number(extra_number);
    if (ext.length() > 0)
        ret += QLatin1Char('.') + ext;

    return ret;
}

QString ShortenFileName(const QString &path, int extra_number)
{
    QString assembled = QStringLiteral("/");
    const QStringList names = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    int cnt = 0;
    for (const QString &s : names) {
        QByteArray encoded = QFile::encodeName(s);
        assembled += (encoded.length() < NAME_MAX) ? s : ShortenName(s, extra_number);
        if (cnt < names.count() - 1)
            assembled += QLatin1Char('/');
        cnt++;
    }

    if (QFile::encodeName(assembled).length() >= PATH_MAX) {
        // still to long, then the Shorten the filename
        assembled = ShortenPath(assembled, extra_number);
    }

    return assembled;
}

Uint64 DiskUsage(const QString &filename)
{
    Uint64 ret = 0;
#ifndef Q_OS_WIN
#ifdef HAVE_STAT64
    struct stat64 sb;
    if (stat64(QFile::encodeName(filename).constData(), &sb) == 0)
#else
    struct stat sb;
    if (stat(QFile::encodeName(filename).constData(), &sb) == 0)
#endif
    {
        ret = (Uint64)sb.st_blocks * 512;
    }
#else
    DWORD high = 0;
    DWORD low = GetCompressedFileSize((LPWSTR)filename.utf16(), &high);
    if (low != INVALID_FILE_SIZE)
        ret = (high * MAXDWORD) + low;
#endif
    return ret;
}

Uint64 DiskUsage(int fd)
{
    Uint64 ret = 0;
#ifndef Q_OS_WIN
#ifdef HAVE_FSTAT64
    struct stat64 sb;
    if (fstat64(fd, &sb) == 0)
#else
    struct stat sb;
    if (fstat(fd, &sb) == 0)
#endif
    {
        ret = (Uint64)sb.st_blocks * 512;
    }
#else
    struct _BY_HANDLE_FILE_INFORMATION info;
    GetFileInformationByHandle((void *)&fd, &info);
    ret = (info.nFileSizeHigh * MAXDWORD) + info.nFileSizeLow;
#endif
    return ret;
}

QSet<QString> AccessibleMountPoints()
{
    QSet<QString> result;
#ifdef Q_OS_LINUX
    FILE *fptr = setmntent("/proc/mounts", "r");
    if (!fptr)
        return result;

    struct mntent mnt;
    char buf[PATH_MAX];
    while (getmntent_r(fptr, &mnt, buf, PATH_MAX)) {
        result.insert(QString::fromUtf8(mnt.mnt_dir));
    }

    endmntent(fptr);

#else
    const QList<Solid::Device> devs = Solid::Device::listFromType(Solid::DeviceInterface::StorageAccess);
    for (const Solid::Device &dev : devs) {
        const Solid::StorageAccess *sa = dev.as<Solid::StorageAccess>();
        if (!sa->filePath().isEmpty() && sa->isAccessible())
            result.insert(sa->filePath());
    }
#endif
    return result;
}

QString MountPoint(const QString &path)
{
    const QSet<QString> mount_points = AccessibleMountPoints();
    QString mount_point;
    for (const QString &mp : mount_points) {
        if (path.startsWith(mp) && (mount_point.isEmpty() || mp.startsWith(mount_point))) {
            mount_point = mp;
        }
    }

    return mount_point;
}

bool IsMounted(const QString &mount_point)
{
    return AccessibleMountPoints().contains(mount_point);
}

QByteArray LoadFile(const QString &path)
{
    QFile fptr(path);
    if (fptr.open(QIODevice::ReadOnly))
        return fptr.readAll();
    else
        throw Error(i18n("Unable to open file %1: %2", path, fptr.errorString()));
}

}
