/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_METADATADOWNLOAD_H
#define BT_METADATADOWNLOAD_H

#include <QByteArray>
#include <util/bitset.h>
#include <util/constants.h>

namespace bt
{
class UTMetaData;

const int METADATA_PIECE_SIZE = 16 * 1024;

/*!
 * \brief Handles the downloading of torrent metadata via the UT metadata extension (BEP 0009).
 */
class MetadataDownload
{
public:
    MetadataDownload(UTMetaData *ext, Uint32 size);
    virtual ~MetadataDownload();

    //! A reject of a piece was received
    void reject(Uint32 piece);

    /*!
        A piece was received
        \return true if all the data has been received
    */
    bool data(Uint32 piece, QByteArrayView piece_data);

    //! Get the result
    [[nodiscard]] const QByteArray &result() const
    {
        return metadata;
    }

private:
    void download(Uint32 piece);
    void downloadNext();

private:
    UTMetaData *ext;
    BitSet pieces;
    QByteArray metadata;
    Uint32 total_size;
};

}

#endif // BT_METADATADOWNLOAD_H
