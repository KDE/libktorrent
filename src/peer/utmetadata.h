/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_UTMETADATA_H
#define BT_UTMETADATA_H

#include <peer/peerprotocolextension.h>

namespace bt
{
class MetadataDownload;
class BDictNode;
class Peer;
class Torrent;

/*!
 * \brief Handles the ut_metadata extension (BEP 0009).
 */
class KTORRENT_EXPORT UTMetaData : public PeerProtocolExtension
{
public:
    UTMetaData(const Torrent &tor, bt::Uint32 id, Peer *peer);
    ~UTMetaData() override;

    /*!
        Handle a metadata packet
    */
    void handlePacket(const bt::Uint8 *packet, Uint32 size) override;

    /*!
        Set the reported metadata size
    */
    void setReportedMetadataSize(Uint32 metadata_size);

private:
    void request(BDictNode *dict);
    void reject(BDictNode *dict);
    void data(BDictNode *dict, QByteArrayView piece_data);
    void sendReject(int piece);
    void sendData(int piece, int total_size, QByteArrayView data);
    void startDownload();

private:
    const Torrent &tor;
    Uint32 reported_metadata_size;
    MetadataDownload *download;
};

}

#endif // BT_UTMETADATA_H
