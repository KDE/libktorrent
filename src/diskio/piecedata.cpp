/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "piecedata.h"

#include <KLocalizedString>

#include <util/error.h>
#include <util/file.h>
#include <util/log.h>
#include <util/sha1hashgen.h>
#include <util/signalcatcher.h>

#include "chunk.h"

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
    if (!ptr) {
        return;
    }

    if (!mapped()) {
        delete[] ptr;
    } else {
        cache_file->unmap(ptr);
    }
    ptr = nullptr;
}

Uint32 PieceData::write(const bt::Uint8 *buf, Uint32 buf_size, Uint32 off)
{
    if (off + buf_size > len || !ptr) {
        return 0;
    }

    if (read_only) {
        throw bt::Error(i18n("Unable to write to a piece mapped read only"));
    }

    WithBusErrorProtection(BusOperation::Write, [&] {
        memcpy(ptr + off, buf, buf_size);
    });
    return buf_size;
}

Uint32 PieceData::read(Uint8 *buf, Uint32 to_read, Uint32 off)
{
    if (off + to_read > len || !ptr) {
        return 0;
    }

    WithBusErrorProtection(BusOperation::Read, [&] {
        memcpy(buf, ptr + off, to_read);
    });
    return to_read;
}

Uint32 PieceData::writeToFile(File &file, Uint32 size, Uint32 off)
{
    if (off + size > len || !ptr) {
        return 0;
    }

    return WithBusErrorProtection(BusOperation::Read, [&] {
        return file.write(ptr + off, size);
    });
}

Uint32 PieceData::readFromFile(File &file, Uint32 size, Uint32 off)
{
    if (off + size > len || !ptr) {
        return 0;
    }

    if (read_only) {
        throw bt::Error(i18n("Unable to write to a piece mapped read only"));
    }

    return WithBusErrorProtection(BusOperation::Write, [&] {
        return file.read(ptr + off, size);
    });
}

void PieceData::updateHash(SHA1HashGen &hg)
{
    if (!ptr) {
        return;
    }

    WithBusErrorProtection(BusOperation::Read, [&] {
        hg.update(ptr, len);
    });
}

SHA1Hash PieceData::generateHash() const
{
    if (!ptr) {
        return SHA1Hash();
    }

    return WithBusErrorProtection(BusOperation::Read, [&] {
        return SHA1Hash::generate(ptr, len);
    });
}

void PieceData::unmapped()
{
    ptr = nullptr;
}
}
