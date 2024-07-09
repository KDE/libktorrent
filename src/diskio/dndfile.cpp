/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "dndfile.h"
#include <klocalizedstring.h>
#include <torrent/torrentfile.h>
#include <util/error.h>
#include <util/file.h>
#include <util/fileops.h>
#include <util/log.h>
#include <util/sha1hash.h>

namespace bt
{
const Uint32 DND_FILE_HDR_MAGIC = 0xD1234567;

struct DNDFileHeader {
    Uint32 magic;
    Uint32 first_size;
    Uint32 last_size;
    Uint8 data_sha1[20];
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
    if (!fptr.open(path, "rb")) {
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
    DNDFileHeader hdr;
    hdr.magic = DND_FILE_HDR_MAGIC;
    hdr.first_size = first_size;
    hdr.last_size = last_size;
    memset(hdr.data_sha1, 0, 20);

    File fptr;
    if (!fptr.open(path, "wb"))
        throw Error(i18n("Cannot create file %1: %2", path, fptr.errorString()));

    fptr.write(&hdr, sizeof(DNDFileHeader));
    fptr.close();
}

Uint32 DNDFile::readFirstChunk(Uint8 *buf, Uint32 off, Uint32 size)
{
    File fptr;
    if (!fptr.open(path, "rb")) {
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
    if (!fptr.open(path, "rb")) {
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
    if (!fptr.open(path, "r+b")) {
        create();
        if (!fptr.open(path, "r+b")) {
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
    if (!fptr.open(path, "r+b")) {
        create();
        if (!fptr.open(path, "r+b")) {
            throw Error(i18n("Failed to write last chunk to DND file: %1", fptr.errorString()));
        }
    }

    fptr.seek(File::BEGIN, sizeof(DNDFileHeader) + first_size + off);
    fptr.write(buf, size);
}

}
