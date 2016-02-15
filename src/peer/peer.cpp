/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include "peer.h"

#include <math.h>
#include <version.h>
#include <util/log.h>
#include <util/functions.h>
#include <net/address.h>
#include <mse/encryptedpacketsocket.h>
#include <diskio/chunk.h>
#include <download/piece.h>
#include <download/request.h>
#include <bcodec/bdecoder.h>
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <torrent/server.h>
#include <torrent/torrent.h>
#include "packetreader.h"
#include "peerdownloader.h"
#include "peeruploader.h"
#include "utpex.h"
#include "peermanager.h"
#include <net/reverseresolver.h>
#include "utmetadata.h"

using namespace net;

namespace bt
{

    static const int MAX_METADATA_SIZE = 4 * 1024 * 1024; // maximum allowed metadata_size (up to 4 MiB)
    static Uint32 peer_id_counter = 1;
    bool Peer::resolve_hostname = true;


    Peer::Peer(mse::EncryptedPacketSocket::Ptr sock,
               const PeerID & peer_id,
               Uint32 num_chunks,
               Uint32 chunk_size,
               Uint32 support,
               bool local,
               ConnectionLimit::Token::Ptr token,
               PeerManager* pman)
        : PeerInterface(peer_id, num_chunks),
          sock(sock),
          token(token),
          pman(pman)
    {
        id = peer_id_counter;
        peer_id_counter++;
        ut_pex_id = 0;

        Uint32 max_packet_len = 9 + MAX_PIECE_LEN; // size of piece packet
        Uint32 bitfield_length = num_chunks / 8 + 1 + (num_chunks % 8 == 0 ? 0 : 1);
        if (bitfield_length > max_packet_len) // bitfield can be longer
            max_packet_len = bitfield_length;

        // to be future proof use 10 times the max packet length,
        // in case some extension message comes along which is larger
        preader = new PacketReader(10*max_packet_len);
        downloader = new PeerDownloader(this, chunk_size);
        uploader = new PeerUploader(this);

        stalled_timer.update();


        connect_time = QTime::currentTime();
        stats.client = peer_id.identifyClient();
        stats.ip_address = getIPAddresss();
        stats.dht_support = support & DHT_SUPPORT;
        stats.fast_extensions = support & FAST_EXT_SUPPORT;
        stats.extension_protocol = support & EXT_PROT_SUPPORT;
        stats.encrypted = sock->encrypted();
        stats.local = local;
        stats.transport_protocol = sock->socketDevice()->transportProtocol();

        if (stats.ip_address == "0.0.0.0")
        {
            Out(SYS_CON | LOG_DEBUG) << "No more 0.0.0.0" << endl;
            kill();
        }
        else
        {
            sock->startMonitoring(preader);
        }

        pex_allowed = stats.extension_protocol;
        extensions.setAutoDelete(true);

        if (resolve_hostname)
        {
            net::ReverseResolver* res = new net::ReverseResolver();
            connect(res, SIGNAL(resolved(QString)), this, SLOT(resolved(QString)), Qt::QueuedConnection);
            res->resolveAsync(sock->getRemoteAddress());
        }
    }


    Peer::~Peer()
    {
        // Seeing that we are going to delete the preader object,
        // call stopMonitoring, in some situations it is possible that the socket
        // is only deleted later because the authenticate object still has a reference to it
        sock->stopMonitoring();
        delete uploader;
        delete downloader;
        delete preader;
    }

    void Peer::closeConnection()
    {
        sock->close();
    }

    void Peer::kill()
    {
        sock->close();
        killed = true;
        token.clear();
    }

    void Peer::handleChoke(Uint32 len)
    {
        if (len != 1)
        {
            kill();
            return;
        }

        if (!stats.choked)
            stats.time_choked = CurrentTime();
        stats.choked = true;
        downloader->choked();
    }

    void Peer::handleUnchoke(Uint32 len)
    {
        if (len != 1)
        {
            Out(SYS_CON | LOG_DEBUG) << "len err UNCHOKE" << endl;
            kill();
            return;
        }

        if (stats.choked)
        {
            stats.time_unchoked = CurrentTime();
            bytes_downloaded_since_unchoke = 0;
        }

        stats.choked = false;
    }

    void Peer::handleInterested(Uint32 len)
    {
        if (len != 1)
        {
            Out(SYS_CON | LOG_DEBUG) << "len err INTERESTED" << endl;
            kill();
            return;
        }

        if (!stats.interested)
        {
            stats.interested = true;
            pman->rerunChoker();
        }

    }

    void Peer::handleNotInterested(Uint32 len)
    {
        if (len != 1)
        {
            kill();
            return;
        }

        if (stats.interested)
        {
            stats.interested = false;
            pman->rerunChoker();
        }
    }

    void Peer::handleHave(const bt::Uint8* packet, Uint32 len)
    {
        if (len != 5)
        {
            kill();
        }
        else
        {
            Uint32 ch = ReadUint32(packet, 1);
            if (ch < pieces.getNumBits())
            {
                pman->have(this, ch);
                pieces.set(ch, true);
            }
            else if (pman->getTorrent().isLoaded())
            {
                Out(SYS_CON | LOG_NOTICE) << "Received invalid have value, kicking peer" << endl;
                kill();
            }
        }
    }

    void Peer::handleHaveAll(Uint32 len)
    {
        if (len != 1)
        {
            kill();
        }
        else
        {
            pieces.setAll(true);
            pman->bitSetReceived(this, pieces);
        }
    }

    void Peer::handleHaveNone(Uint32 len)
    {
        if (len != 1)
        {
            kill();
        }
        else
        {
            pieces.setAll(false);
            pman->bitSetReceived(this, pieces);
        }
    }

    void Peer::handleBitField(const bt::Uint8* packet, Uint32 len)
    {
        if (len != 1 + pieces.getNumBytes())
        {
            if (pman->getTorrent().isLoaded())
                kill();
        }
        else
        {
            pieces = BitSet(packet + 1, pieces.getNumBits());
            pman->bitSetReceived(this, pieces);
        }
    }

    void Peer::handleRequest(const bt::Uint8* packet, Uint32 len)
    {
        if (len != 13)
        {
            kill();
            return;
        }

        Request r(
                    ReadUint32(packet, 1),
                    ReadUint32(packet, 5),
                    ReadUint32(packet, 9),
                    downloader);

        if (stats.has_upload_slot)
            uploader->addRequest(r);
        else if (stats.fast_extensions)
            sendReject(r);
    }

    void Peer::handlePiece(const bt::Uint8* packet, Uint32 len)
    {
        if (paused)
            return;

        if (len < 9)
        {
            kill();
            return;
        }

        snub_timer.update();

        stats.bytes_downloaded += (len - 9);
        bytes_downloaded_since_unchoke += (len - 9);
        Piece p(ReadUint32(packet, 1),
                ReadUint32(packet, 5),
                len - 9, downloader, packet + 9);
        downloader->piece(p);
        pman->pieceReceived(p);
        downloader->update();
    }

    void Peer::handleCancel(const bt::Uint8* packet, Uint32 len)
    {
        if (len != 13)
        {
            kill();
        }
        else
        {
            Request r(ReadUint32(packet, 1),
                      ReadUint32(packet, 5),
                      ReadUint32(packet, 9),
                      downloader);
            uploader->removeRequest(r);
            sock->doNotSendPiece(r, stats.fast_extensions);
        }
    }

    void Peer::handleReject(const bt::Uint8* packet, Uint32 len)
    {
        if (len != 13)
        {
            kill();
        }
        else
        {
            Request r(ReadUint32(packet, 1),
                      ReadUint32(packet, 5),
                      ReadUint32(packet, 9),
                      downloader);
            downloader->onRejected(r);
        }
    }

    void Peer::handlePort(const bt::Uint8* packet, Uint32 len)
    {
        if (len != 3)
        {
            kill();
        }
        else
        {
            Uint16 port = ReadUint16(packet, 1);
            //	Out(SYS_CON|LOG_DEBUG) << "Got PORT packet : " << port << endl;
            pman->portPacketReceived(getIPAddresss(), port);
        }
    }

    void Peer::handlePacket(const bt::Uint8* packet, Uint32 size)
    {
        if (killed || size == 0)
            return;

        switch (packet[0])
        {
        case CHOKE:
            handleChoke(size);
            break;
        case UNCHOKE:
            handleUnchoke(size);
            break;
        case INTERESTED:
            handleInterested(size);
            break;
        case NOT_INTERESTED:
            handleNotInterested(size);
            break;
        case HAVE:
            handleHave(packet, size);
            break;
        case BITFIELD:
            handleBitField(packet, size);
            break;
        case REQUEST:
            handleRequest(packet, size);
            break;
        case PIECE:
            handlePiece(packet, size);
            break;
        case CANCEL:
            handleCancel(packet, size);
            break;
        case REJECT_REQUEST:
            handleReject(packet, size);
            break;
        case PORT:
            handlePort(packet, size);
            break;
        case HAVE_ALL:
            handleHaveAll(size);
            break;
        case HAVE_NONE:
            handleHaveNone(size);
            break;
        case SUGGEST_PIECE:
            // ignore suggestions for the moment
            break;
        case ALLOWED_FAST:
            // we no longer support this, so do nothing
            break;
        case EXTENDED:
            handleExtendedPacket(packet, size);
            break;
        }
    }

    void Peer::pause()
    {
        if (paused)
            return;

        downloader->cancelAll();
        // choke the peer and tell it we are not interested
        choke();
        sendNotInterested();
        paused = true;
    }

    void Peer::unpause()
    {
        paused = false;
    }

    void Peer::handleExtendedPacket(const Uint8* packet, Uint32 size)
    {
        if (size <= 2)
            return;

        PeerProtocolExtension* ext = extensions.find(packet[1]);
        if (ext)
        {
            ext->handlePacket(packet, size);
        }
        else if (packet[1] == 0)
        {
            handleExtendedHandshake(packet, size);
        }
    }

    void Peer::handleExtendedHandshake(const Uint8* packet, Uint32 size)
    {
        QByteArray tmp = QByteArray::fromRawData((const char*)packet, size);
        BNode* node = 0;
        try
        {
            BDecoder dec(tmp, false, 2);
            node = dec.decode();
            if (!node || node->getType() != BNode::DICT)
            {
                delete node;
                return;
            }

            BDictNode* dict = (BDictNode*)node;
            BDictNode* mdict = dict->getDict(QByteArrayLiteral("m"));
            if (!mdict)
            {
                delete node;
                return;
            }

            BValueNode* val = 0;

            if ((val = mdict->getValue(QByteArrayLiteral("ut_pex"))) && UTPex::isEnabled())
            {
                // ut_pex packet
                ut_pex_id = val->data().toInt();
                if (ut_pex_id == 0)
                {
                    extensions.erase(UT_PEX_ID);
                }
                else
                {
                    PeerProtocolExtension* ext = extensions.find(UT_PEX_ID);
                    if (ext)
                        ext->changeID(ut_pex_id);
                    else if (pex_allowed)
                        extensions.insert(UT_PEX_ID, new UTPex(this, ut_pex_id));
                }
            }

            if ((val = mdict->getValue(QByteArrayLiteral("ut_metadata"))))
            {
                // meta data
                Uint32 ut_metadata_id = val->data().toInt();
                if (ut_metadata_id == 0) // disabled by other side
                {
                    extensions.erase(UT_METADATA_ID);
                }
                else
                {
                    PeerProtocolExtension* ext = extensions.find(UT_METADATA_ID);
                    if (ext)
                    {
                        ext->changeID(ut_metadata_id);
                    }
                    else
                    {
                        int metadata_size = 0;
                        if (dict->getValue(QByteArrayLiteral("metadata_size")))
                            metadata_size = dict->getInt(QByteArrayLiteral("metadata_size"));

                        if (metadata_size > MAX_METADATA_SIZE) // check for wrong metadata_size values
                        {
                            Out(SYS_GEN | LOG_NOTICE) << "Wrong metadata size: " << metadata_size << ". Killing peer... " << endl;
                            kill();
                            return;
                        }

                        UTMetaData* md = new UTMetaData(pman->getTorrent(), ut_metadata_id, this);
                        md->setReportedMetadataSize(metadata_size);
                        extensions.insert(UT_METADATA_ID, md);
                    }
                }
            }

            if ((val = dict->getValue(QByteArrayLiteral("reqq"))))
            {
                stats.max_request_queue = val->data().toInt();
            }

            if ((val = dict->getValue(QByteArrayLiteral("upload_only"))))
            {
                stats.partial_seed = val->data().toInt() == 1;
            }
        }
        catch (...)
        {
            // just ignore invalid packets
            Out(SYS_CON | LOG_DEBUG) << "Invalid extended packet" << endl;
        }
        delete node;
    }

    Uint32 Peer::sendData(const Uint8* data, Uint32 len)
    {
        if (killed) return 0;

        Uint32 ret = sock->sendData(data, len);
        if (!sock->ok())
            kill();

        return ret;
    }

    Uint32 Peer::readData(Uint8* buf, Uint32 len)
    {
        if (killed) return 0;

        Uint32 ret = sock->readData(buf, len);

        if (!sock->ok())
            kill();

        return ret;
    }

    Uint32 Peer::bytesAvailable() const
    {
        return sock->bytesAvailable();
    }

    Uint32 Peer::getUploadRate() const
    {
        if (sock)
            return sock->getUploadRate();
        else
            return 0;
    }

    Uint32 Peer::getDownloadRate() const
    {
        if (sock)
            return sock->getDownloadRate();
        else
            return 0;
    }

    void Peer::update()
    {
        if (killed)
            return;

        if (!sock->ok() || !preader->ok())
        {
            Out(SYS_CON | LOG_DEBUG) << "Connection closed" << endl;
            kill();
            return;
        }

        sock->updateSpeeds(bt::CurrentTime());
        preader->update(*this);

        Uint32 data_bytes = sock->dataBytesUploaded();

        if (data_bytes > 0)
        {
            stats.bytes_uploaded += data_bytes;
            uploader->addUploadedBytes(data_bytes);
        }

        if (!paused)
        {
            PtrMap<Uint32, PeerProtocolExtension>::iterator i = extensions.begin();
            while (i != extensions.end())
            {
                if (i->second->needsUpdate())
                    i->second->update();
                i++;
            }
        }

        // if no data is being sent or recieved, and there are pending requests
        // increment the connection stalled timer
        if (getUploadRate() > 100 || getDownloadRate() > 100 ||
                (uploader->getNumRequests() == 0 && sock->numPendingPieceUploads() == 0 && downloader->getNumRequests() == 0))
            stalled_timer.update();

        stats.download_rate = this->getDownloadRate();
        stats.upload_rate = this->getUploadRate();
        stats.perc_of_file = this->percentAvailable();
        stats.snubbed = this->isSnubbed();
        stats.num_up_requests = uploader->getNumRequests() + sock->numPendingPieceUploads();
        stats.num_down_requests = downloader->getNumRequests();
    }

    bool Peer::isStalled() const
    {
        return stalled_timer.getElapsedSinceUpdate() >= 2*60*1000;
    }

    bool Peer::isSnubbed() const
    {
        // 4 minutes
        return snub_timer.getElapsedSinceUpdate() >= 2*60*1000 && stats.num_down_requests > 0;
    }

    QString Peer::getIPAddresss() const
    {
        if (sock)
            return sock->getRemoteIPAddress();
        else
            return QString();
    }

    Uint16 Peer::getPort() const
    {
        if (!sock)
            return 0;
        else
            return sock->getRemotePort();
    }

    net::Address Peer::getAddress() const
    {
        if (!sock)
            return net::Address();
        else
            return sock->getRemoteAddress();
    }

    Uint32 Peer::getTimeSinceLastPiece() const
    {
        return snub_timer.getElapsedSinceUpdate();
    }

    float Peer::percentAvailable() const
    {
        // calculation needs to use bytes, instead of chunks, because
        // the last chunk can have a different size
        const Torrent & tor = pman->getTorrent();
        Uint64 bytes = 0;
        if (pieces.get(tor.getNumChunks() - 1))
            bytes = tor.getChunkSize() * (pieces.numOnBits() - 1) + tor.getLastChunkSize();
        else
            bytes = tor.getChunkSize() * pieces.numOnBits();

        Uint64 tbytes = tor.getChunkSize() * (pieces.getNumBits() - 1) + tor.getLastChunkSize();
        return (float)bytes / (float)tbytes * 100.0;
    }

    void Peer::setACAScore(double s)
    {
        stats.aca_score = s;
    }

    void Peer::choke()
    {
        if (!stats.has_upload_slot)
            return;

        sendChoke();
        uploader->clearAllRequests();
    }

    void Peer::emitPortPacket()
    {
        pman->portPacketReceived(sock->getRemoteIPAddress(), sock->getRemotePort());
    }

    void Peer::emitPex(const QByteArray & data)
    {
        pman->pex(data);
    }

    void Peer::setPexEnabled(bool on)
    {
        if (!stats.extension_protocol)
            return;

        PeerProtocolExtension* ext = extensions.find(UT_PEX_ID);
        if (ext && (!on || !UTPex::isEnabled()))
        {
            extensions.erase(UT_PEX_ID);
        }
        else if (!ext && on && ut_pex_id > 0 && UTPex::isEnabled())
        {
            // if the other side has enabled it to, create a new UTPex object
            extensions.insert(UT_PEX_ID, new UTPex(this, ut_pex_id));
        }

        pex_allowed = on;
    }

    void Peer::sendExtProtHandshake(Uint16 port, Uint32 metadata_size, bool partial_seed)
    {
        if (!stats.extension_protocol)
            return;

        QByteArray arr;
        BEncoder enc(new BEncoderBufferOutput(arr));
        enc.beginDict();
        enc.write(QByteArrayLiteral("m"));
        // supported messages
        enc.beginDict();
        enc.write(QByteArrayLiteral("ut_pex"));enc.write((Uint32)(pex_allowed ? UT_PEX_ID : 0));
        enc.write(QByteArrayLiteral("ut_metadata")); enc.write(UT_METADATA_ID);
        enc.end();
        if (port > 0)
        {
            enc.write(QByteArrayLiteral("p"));
            enc.write((Uint32)port);
        }
        enc.write(QByteArrayLiteral("reqq"));
        enc.write((bt::Uint32)250);
        if (metadata_size)
        {
            enc.write(QByteArrayLiteral("metadata_size"));
            enc.write(metadata_size);
        }

        enc.write(QByteArrayLiteral("upload_only"), partial_seed ? QByteArrayLiteral("1") : QByteArrayLiteral("0"));
        enc.write(QByteArrayLiteral("v")); enc.write(bt::GetVersionString().toLatin1());
        enc.end();
        sendExtProtMsg(0, arr);
    }


    void Peer::setGroupIDs(Uint32 up_gid, Uint32 down_gid)
    {
        sock->setGroupID(up_gid, true);
        sock->setGroupID(down_gid, false);
    }

    void Peer::resolved(const QString & hinfo)
    {
        stats.hostname = hinfo;
    }

    void Peer::setResolveHostnames(bool on)
    {
        resolve_hostname = on;
    }

    void Peer::emitMetadataDownloaded(const QByteArray& data)
    {
        emit metadataDownloaded(data);
    }

    bool Peer::hasWantedChunks(const bt::BitSet& wanted_chunks) const
    {
        BitSet bs = pieces;
        bs.andBitSet(wanted_chunks);
        return bs.numOnBits() > 0;
    }

    Uint32 Peer::averageDownloadSpeed() const
    {
        if (stats.choked)
            return 0;

        TimeStamp now = CurrentTime();
        if (now >= getUnchokeTime())
            return 0;
        else
            return bytes_downloaded_since_unchoke / (getUnchokeTime() - now);
    }


    void Peer::sendChoke()
    {
        if (!stats.has_upload_slot)
            return;

        sock->addPacket(Packet::Ptr(new Packet(CHOKE)));
        stats.has_upload_slot = false;
    }

    void Peer::sendUnchoke()
    {
        if (stats.has_upload_slot)
            return;

        sock->addPacket(Packet::Ptr(new Packet(UNCHOKE)));
        stats.has_upload_slot = true;
    }

    void Peer::sendEvilUnchoke()
    {
        sock->addPacket(Packet::Ptr(new Packet(UNCHOKE)));
        stats.has_upload_slot = false;
    }

    void Peer::sendInterested()
    {
        if (stats.am_interested == true)
            return;

        sock->addPacket(Packet::Ptr(new Packet(INTERESTED)));
        stats.am_interested = true;
    }

    void Peer::sendNotInterested()
    {
        if (stats.am_interested == false)
            return;

        sock->addPacket(Packet::Ptr(new Packet(NOT_INTERESTED)));
        stats.am_interested = false;
    }

    void Peer::sendRequest(const Request & r)
    {
        sock->addPacket(Packet::Ptr(new Packet(r, bt::REQUEST)));
    }

    void Peer::sendCancel(const Request & r)
    {
        sock->addPacket(Packet::Ptr(new Packet(r, bt::CANCEL)));
    }

    void Peer::sendReject(const Request & r)
    {
        sock->addPacket(Packet::Ptr(new Packet(r, bt::REJECT_REQUEST)));
    }

    void Peer::sendHave(Uint32 index)
    {
        sock->addPacket(Packet::Ptr(new Packet(index, bt::HAVE)));
    }

    void Peer::sendPort(Uint16 port)
    {
        sock->addPacket(Packet::Ptr(new Packet(port)));
    }

    void Peer::sendBitSet(const BitSet & bs)
    {
        sock->addPacket(Packet::Ptr(new Packet(bs)));
    }

    void Peer::sendHaveAll()
    {
        sock->addPacket(Packet::Ptr(new Packet(bt::HAVE_ALL)));
    }

    void Peer::sendHaveNone()
    {
        sock->addPacket(Packet::Ptr(new Packet(bt::HAVE_NONE)));
    }

    void Peer::sendSuggestPiece(Uint32 index)
    {
        sock->addPacket(Packet::Ptr(new Packet(index, bt::SUGGEST_PIECE)));
    }

    void Peer::sendAllowedFast(Uint32 index)
    {
        sock->addPacket(Packet::Ptr(new Packet(index, bt::ALLOWED_FAST)));
    }

    bool Peer::sendChunk(Uint32 index, Uint32 begin, Uint32 len, Chunk * ch)
    {
        //		Out() << "sendChunk " << index << " " << begin << " " << len << endl;
        if (begin >= ch->getSize() || begin + len > ch->getSize())
        {
            Out(SYS_CON | LOG_NOTICE) << "Warning : Illegal piece request" << endl;
            Out(SYS_CON | LOG_NOTICE) << "\tChunk : index " << index << " size = " << ch->getSize() << endl;
            Out(SYS_CON | LOG_NOTICE) << "\tPiece : begin = " << begin << " len = " << len << endl;
            return false;
        }
        else if (!ch)
        {
            Out(SYS_CON | LOG_NOTICE) << "Warning : attempted to upload an invalid chunk" << endl;
            return false;
        }
        else
        {
            /*		Out(SYS_CON|LOG_DEBUG) << QString("Uploading %1 %2 %3 %4 %5")
                *			.arg(index).arg(begin).arg(len).arg((quint64)ch,0,16).arg((quint64)ch->getData(),0,16)
                *			<< endl;;
                */
            sock->addPacket(Packet::Ptr(new Packet(index, begin, len, ch)));
            return true;
        }
    }

    void Peer::sendExtProtMsg(Uint8 id, const QByteArray & data)
    {
        sock->addPacket(Packet::Ptr(new Packet(id, data)));
    }

    void Peer::clearPendingPieceUploads()
    {
        sock->clearPieces(stats.fast_extensions);
    }

    void Peer::chunkAllowed(Uint32 chunk)
    {
        sendHave(chunk);
    }

}
