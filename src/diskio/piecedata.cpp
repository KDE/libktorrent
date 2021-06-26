/***************************************************************************
 *   Copyright (C) 2008 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
 *   ivasic@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include "piecedata.h"
#include <klocalizedstring.h>
#include <util/log.h>
#ifndef Q_WS_WIN
#include <util/signalcatcher.h>
#endif
#include "chunk.h"
#include <util/error.h>
#include <util/file.h>
#include <util/sha1hashgen.h>

namespace bt
{
PieceData::PieceData(bt::Chunk *chunk, bt::Uint32 off, bt::Uint32 len, bt::Uint8 *ptr, bt::CacheFile::Ptr cache_file, bool read_only)
    : chunk(chunk)
    , off(off)
    , len(len)
    , ptr(ptr)
    , cache_file(cache_file)
    , read_only(read_only)
{
}

PieceData::~PieceData()
{
    unload();
}

void PieceData::unload()
{
    if (!ptr)
        return;

    if (!mapped())
        delete[] ptr;
    else
        cache_file->unmap(ptr, len);
    ptr = nullptr;
}

Uint32 PieceData::write(const bt::Uint8 *buf, Uint32 buf_size, Uint32 off)
{
    if (off + buf_size > len || !ptr)
        return 0;

    if (read_only)
        throw bt::Error(i18n("Unable to write to a piece mapped read only"));

#ifndef Q_WS_WIN
    BUS_ERROR_WPROTECT();
#endif
    memcpy(ptr + off, buf, buf_size);
    return buf_size;
}

Uint32 PieceData::read(Uint8 *buf, Uint32 to_read, Uint32 off)
{
    if (off + to_read > len || !ptr)
        return 0;

#ifndef Q_WS_WIN
    BUS_ERROR_RPROTECT();
#endif
    memcpy(buf, ptr + off, to_read);
    return to_read;
}

Uint32 PieceData::writeToFile(File &file, Uint32 size, Uint32 off)
{
    if (off + size > len || !ptr)
        return 0;

#ifndef Q_WS_WIN
    BUS_ERROR_RPROTECT();
#endif
    return file.write(ptr + off, size);
}

Uint32 PieceData::readFromFile(File &file, Uint32 size, Uint32 off)
{
    if (off + size > len || !ptr)
        return 0;

    if (read_only)
        throw bt::Error(i18n("Unable to write to a piece mapped read only"));

#ifndef Q_WS_WIN
    BUS_ERROR_WPROTECT();
#endif
    return file.read(ptr + off, size);
}

void PieceData::updateHash(SHA1HashGen &hg)
{
    if (!ptr)
        return;

#ifndef Q_WS_WIN
    BUS_ERROR_RPROTECT();
#endif
    hg.update(ptr, len);
}

SHA1Hash PieceData::generateHash() const
{
    if (!ptr)
        return SHA1Hash();

#ifndef Q_WS_WIN
    BUS_ERROR_RPROTECT();
#endif
    return SHA1Hash::generate(ptr, len);
}

void PieceData::unmapped()
{
    ptr = nullptr;
}
}
