/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "file.h"

#include <config-ktorrent.h>

#include "error.h"
#include "log.h"
#include <errno.h>
#include <klocalizedstring.h>
#include <qfile.h>
#include <stdio.h>
#include <string.h>

namespace bt
{
File::File()
    : fptr(nullptr)
{
}

File::~File()
{
    close();
}

bool File::open(const QString &file, const QString &mode)
{
    this->file = file;
    if (fptr)
        close();

#ifdef HAVE_FOPEN64
    fptr = fopen64(QFile::encodeName(file).constData(), mode.toUtf8().constData());
#else
    fptr = fopen(QFile::encodeName(file).constData(), mode.toUtf8().constData());
#endif
    return fptr != nullptr;
}

void File::close()
{
    if (fptr) {
        fclose(fptr);
        fptr = nullptr;
    }
}

void File::flush()
{
    if (fptr)
        fflush(fptr);
}

Uint32 File::write(const void *buf, Uint32 size)
{
    if (!fptr)
        return 0;

    Uint32 ret = fwrite(buf, 1, size, fptr);
    if (ret != size) {
        if (errno == ENOSPC)
            Out(SYS_DIO | LOG_IMPORTANT) << "Disk full !" << endl;

        throw Error(i18n("Cannot write to %1: %2", file, QString::fromUtf8(strerror(errno))));
    }
    return ret;
}

Uint32 File::read(void *buf, Uint32 size)
{
    if (!fptr)
        return 0;

    Uint32 ret = fread(buf, 1, size, fptr);
    if (ferror(fptr)) {
        clearerr(fptr);
        throw Error(i18n("Cannot read from %1", file));
    }
    return ret;
}

Uint64 File::seek(SeekPos from, Int64 num)
{
    //  printf("sizeof(off_t) = %i\n",sizeof(__off64_t));
    if (!fptr)
        return 0;

    int p = SEEK_CUR; // use a default to prevent compiler warning
    switch (from) {
    case BEGIN:
        p = SEEK_SET;
        break;
    case END:
        p = SEEK_END;
        break;
    case CURRENT:
        p = SEEK_CUR;
        break;
    default:
        break;
    }
#ifdef HAVE_FSEEKO64
    fseeko64(fptr, num, p);
    return ftello64(fptr);
#elif HAVE_FSEEKO
    fseeko(fptr, num, p);
    return ftello(fptr);
#else
    fseek(fptr, num, p);
    return ftell(fptr);
#endif
}

bool File::eof() const
{
    if (!fptr)
        return true;

    return feof(fptr) != 0;
}

Uint64 File::tell() const
{
    if (!fptr)
        return 0;
#ifdef HAVE_FTELLO64
    return ftello64(fptr);
#elif HAVE_FTELLO
    return ftello(fptr);
#else
    return ftell(fptr);
#endif
}

QString File::errorString() const
{
    return QString::fromUtf8(strerror(errno));
}
}
