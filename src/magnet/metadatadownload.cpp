/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "metadatadownload.h"
#include <bcodec/bencoder.h>
#include <peer/utmetadata.h>
#include <util/log.h>

namespace bt
{
MetadataDownload::MetadataDownload(UTMetaData *ext, Uint32 size)
    : ext(ext)
    , total_size(size)
{
    metadata.resize(size);
    Uint32 num_pieces = size / METADATA_PIECE_SIZE;
    if (size % METADATA_PIECE_SIZE > 0) {
        num_pieces++;
    }

    pieces = BitSet(num_pieces);
    download(0);
}

MetadataDownload::~MetadataDownload()
{
}

void MetadataDownload::reject(Uint32 piece)
{
    Out(SYS_GEN | LOG_NOTICE) << "Metadata download, piece " << piece << " rejected" << endl;
    downloadNext();
}

bool MetadataDownload::data(Uint32 piece, QByteArrayView piece_data)
{
    // validate data
    if (piece >= pieces.getNumBits()) {
        Out(SYS_GEN | LOG_NOTICE) << "Metadata download, received piece " << piece << " is invalid " << endl;
        downloadNext();
        return false;
    }

    int size = METADATA_PIECE_SIZE;
    if (piece == pieces.getNumBits() - 1 && total_size % METADATA_PIECE_SIZE > 0) {
        size = total_size % METADATA_PIECE_SIZE;
    }

    if (size != piece_data.size()) {
        Out(SYS_GEN | LOG_NOTICE) << "Metadata download, received piece " << piece << " has the wrong size " << endl;
        downloadNext();
        return false;
    }

    // piece fits, so copy into data
    // Out(SYS_GEN|LOG_NOTICE) << "Metadata download, dowloaded " << piece << endl;
    Uint32 off = piece * METADATA_PIECE_SIZE;
    if (static_cast<size_t>(metadata.size()) < off + size) {
        Out(SYS_GEN | LOG_NOTICE) << "Metadata download, received large piece " << (off + size) << ", max " << metadata.size() << endl;
        metadata.resize(off + size);
    }
    memcpy(metadata.data() + off, piece_data.data(), size);
    pieces.set(piece, true);

    if (!pieces.allOn()) {
        downloadNext();
    }

    return pieces.allOn();
}

void MetadataDownload::download(Uint32 piece)
{
    QByteArray request;
    BEncoder enc(std::make_unique<BEncoderBufferOutput>(request));
    enc.beginDict();
    enc.write(QByteArrayLiteral("msg_type"));
    enc.write((bt::Uint32)0);
    enc.write(QByteArrayLiteral("piece"));
    enc.write((bt::Uint32)piece);
    enc.end();
    ext->sendPacket(request);
}

void MetadataDownload::downloadNext()
{
    Out(SYS_GEN | LOG_NOTICE) << "Metadata download, requesting " << pieces.getNumBits() << " pieces" << endl;
    for (Uint32 i = 0; i < pieces.getNumBits(); i++) {
        if (!pieces.get(i)) {
            download(i);
        }
    }
}

}
