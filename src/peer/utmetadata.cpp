/*
    SPDX-FileCopyrightText: 2009 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "utmetadata.h"
#include "peer.h"
#include <QByteArray>
#include <bcodec/bdecoder.h>
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <magnet/metadatadownload.h>
#include <torrent/torrent.h>
#include <util/log.h>

namespace bt
{
UTMetaData::UTMetaData(const Torrent &tor, bt::Uint32 id, Peer *peer)
    : PeerProtocolExtension(id, peer)
    , tor(tor)
    , reported_metadata_size(0)
    , download(nullptr)
{
}

UTMetaData::~UTMetaData()
{
}

void UTMetaData::handlePacket(const bt::Uint8 *packet, Uint32 size)
{
    QByteArray tmp = QByteArray::fromRawData((const char *)packet, size);
    BNode *node = nullptr;
    try {
        BDecoder dec(tmp, false, 2);
        node = dec.decode();
        if (!node || node->getType() != BNode::DICT) {
            delete node;
            return;
        }

        BDictNode *dict = (BDictNode *)node;
        int type = dict->getInt(QByteArrayLiteral("msg_type"));
        switch (type) {
        case 0: // request
            request(dict);
            break;
        case 1: { // data
            data(dict, tmp.mid(dec.position()));
            break;
        }
        case 2: // reject
            reject(dict);
            break;
        }
    } catch (...) {
        Out(SYS_CON | LOG_DEBUG) << "Invalid metadata packet" << endl;
    }
    delete node;
}

void UTMetaData::data(BDictNode *dict, const QByteArray &piece_data)
{
    if (download) {
        if (download->data(dict->getInt(QByteArrayLiteral("piece")), piece_data)) {
            peer->emitMetadataDownloaded(download->result());
        }
    }
}

void UTMetaData::reject(BDictNode *dict)
{
    if (download)
        download->reject(dict->getInt(QByteArrayLiteral("piece")));
}

void UTMetaData::request(BDictNode *dict)
{
    int piece = dict->getInt(QByteArrayLiteral("piece"));
    Out(SYS_CON | LOG_DEBUG) << "Received request for metadata piece " << piece << endl;
    if (!tor.isLoaded()) {
        sendReject(piece);
        return;
    }

    const QByteArray &md = tor.getMetaData();
    int num_pieces = md.size() / METADATA_PIECE_SIZE + (md.size() % METADATA_PIECE_SIZE == 0 ? 0 : 1);
    if (piece < 0 || piece >= num_pieces) {
        sendReject(piece);
        return;
    }

    int off = piece * METADATA_PIECE_SIZE;
    int last_len = (md.size() % METADATA_PIECE_SIZE == 0) ? METADATA_PIECE_SIZE : (md.size() % METADATA_PIECE_SIZE);
    int len = piece == num_pieces - 1 ? last_len : METADATA_PIECE_SIZE;
    sendData(piece, md.size(), md.mid(off, len));
}

void UTMetaData::sendReject(int piece)
{
    QByteArray data;
    BEncoder enc(new BEncoderBufferOutput(data));
    enc.beginDict();
    enc.write(QByteArrayLiteral("msg_type"));
    enc.write((bt::Uint32)2);
    enc.write(QByteArrayLiteral("piece"));
    enc.write((bt::Uint32)piece);
    enc.end();
    sendPacket(data);
}

void UTMetaData::sendData(int piece, int total_size, const QByteArray &data)
{
    Out(SYS_CON | LOG_DEBUG) << "Sending metadata piece " << piece << endl;
    QByteArray tmp;
    BEncoder enc(new BEncoderBufferOutput(tmp));
    enc.beginDict();
    enc.write(QByteArrayLiteral("msg_type"));
    enc.write((bt::Uint32)1);
    enc.write(QByteArrayLiteral("piece"));
    enc.write((bt::Uint32)piece);
    enc.write(QByteArrayLiteral("total_size"));
    enc.write((bt::Uint32)total_size);
    enc.end();
    tmp.append(data);
    sendPacket(tmp);
}

void UTMetaData::setReportedMetadataSize(Uint32 metadata_size)
{
    reported_metadata_size = metadata_size;
    if (reported_metadata_size > 0 && !tor.isLoaded() && !download) {
        download = new MetadataDownload(this, reported_metadata_size);
    }
}

}
