/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "dndfile.h"
#include <KLocalizedString>
#include <torrent/torrentfile.h>
#include <util/error.h>
#include <util/file.h>
#include <util/fileops.h>
#include <util/log.h>
#include <util/sha1hash.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
constexpr Uint32 DND_FILE_HDR_MAGIC = 0xD1234567;

/*!
 * \brief Header of a DND file, contains the size of the first and last chunks.
 */
struct DNDFileHeader {
    Uint32 magic = DND_FILE_HDR_MAGIC;
    Uint32 first_size;
    Uint32 last_size;
    Uint8 data_sha1[20] = {0};
};

DNDFile::DNDFile(const QString &path, const TorrentFile *tf, Uint32 chunk_size)
    : path(path)
    , first_size(chunk_size - tf->getFirstChunkOffset())
    , last_size(tf->getLastChunkSize())
{
}

DNDFile::~DNDFile()
{
}

void DNDFile::changePath(const QString &npath)
{
    path = npath;
}

void DNDFile::checkIntegrity()
{
    File fptr;
    if (!fptr.open(path, u"rb"_s)) {
        create();
        return;
    }

    DNDFileHeader hdr;
    if (fptr.read(&hdr, sizeof(DNDFileHeader)) != sizeof(DNDFileHeader)) {
        create();
        return;
    }

    if (hdr.magic != DND_FILE_HDR_MAGIC) {
        create();
        return;
    }
}

void DNDFile::create()
{
    const DNDFileHeader hdr{
        .first_size = first_size,
        .last_size = last_size,
    };

    File fptr;
    if (!fptr.open(path, u"wb"_s))
        throw Error(i18n("Cannot create file %1: %2", path, fptr.errorString()));

    fptr.write(&hdr, sizeof(DNDFileHeader));
    fptr.close();
}

Uint32 DNDFile::readFirstChunk(Uint8 *buf, Uint32 off, Uint32 size)
{
    File fptr;
    if (!fptr.open(path, u"rb"_s)) {
        create();
        return 0;
    }

    Uint64 read_pos = sizeof(DNDFileHeader) + off;
    if (fptr.seek(File::BEGIN, read_pos) != read_pos)
        return 0;

    return fptr.read(buf, size);
}

Uint32 DNDFile::readLastChunk(Uint8 *buf, Uint32 off, Uint32 size)
{
    File fptr;
    if (!fptr.open(path, u"rb"_s)) {
        create();
        return 0;
    }

    Uint64 read_pos = sizeof(DNDFileHeader) + first_size + off;
    if (fptr.seek(File::BEGIN, read_pos) != read_pos)
        return 0;

    return fptr.read(buf, size);
}

void DNDFile::writeFirstChunk(const Uint8 *buf, Uint32 off, Uint32 size)
{
    File fptr;
    if (!fptr.open(path, u"r+b"_s)) {
        create();
        if (!fptr.open(path, u"r+b"_s)) {
            throw Error(i18n("Failed to write first chunk to DND file: %1", fptr.errorString()));
        }
    }

    // write data
    fptr.seek(File::BEGIN, sizeof(DNDFileHeader) + off);
    fptr.write(buf, size);
}

void DNDFile::writeLastChunk(const Uint8 *buf, Uint32 off, Uint32 size)
{
    File fptr;
    if (!fptr.open(path, u"r+b"_s)) {
        create();
        if (!fptr.open(path, u"r+b"_s)) {
            throw Error(i18n("Failed to write last chunk to DND file: %1", fptr.errorString()));
        }
    }

    fptr.seek(File::BEGIN, sizeof(DNDFileHeader) + first_size + off);
    fptr.write(buf, size);
}

}
