/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEER_H
#define BTPEER_H

#include "connectionlimit.h"
#include "peerid.h"
#include "peerprotocolextension.h"
#include <QDateTime>
#include <QObject>
#include <interfaces/peerinterface.h>
#include <ktorrent_export.h>
#include <mse/encryptedpacketsocket.h>
#include <util/ptrmap.h>
#include <util/timer.h>

namespace net
{
class Address;
}

namespace bt
{
class Peer;
class Piece;
class PacketReader;
class PeerDownloader;
class PeerUploader;
class PeerManager;
class BitSet;

/*!
 * \author Joris Guisson
 * \brief Manages the connection with a peer
 *
 * This class manages a connection with a peer in the P2P network.
 * It provides functions for sending packets. Packets it receives
 * get relayed to the outside world using a bunch of signals.
 */
class KTORRENT_EXPORT Peer : public QObject, public PeerInterface
{
    Q_OBJECT
public:
    /*!
     * Constructor, set the socket.
     * The socket is already opened.
     * \param sock The socket
     * \param peer_id The Peer's BitTorrent ID
     * \param num_chunks The number of chunks in the file
     * \param chunk_size Size of each chunk
     * \param support Which extensions the peer supports
     * \param local Whether or not it is a local peer
     * \param token ConnectionLimit token
     * \param pman The PeerManager
     */
    Peer(mse::EncryptedPacketSocket::Ptr sock,
         const PeerID &peer_id,
         Uint32 num_chunks,
         Uint32 chunk_size,
         Uint32 support,
         bool local,
         ConnectionLimit::Token::Ptr token,
         PeerManager *pman);

    ~Peer() override;

    //! Get the peer's unique ID.
    Uint32 getID() const
    {
        return id;
    }

    //! Get the IP address of the Peer.
    QString getIPAddresss() const;

    //! Get the port of the Peer
    Uint16 getPort() const;

    //! Get the address of the peer
    net::Address getAddress() const;

    //! See if the peer is stalled.
    bool isStalled() const;

    //! Are we being snubbed by the Peer
    bool isSnubbed() const;

    //! Get the upload rate in bytes per sec
    Uint32 getUploadRate() const;

    //! Get the download rate in bytes per sec
    Uint32 getDownloadRate() const;

    //! Update the up- and down- speed and handle incoming packets
    void update();

    //! Pause the peer connection
    void pause();

    //! Unpause the peer connection
    void unpause();

    //! Get the PeerDownloader.
    PeerDownloader *getPeerDownloader() const
    {
        return downloader;
    }

    //! Get the PeerUploader.
    PeerUploader *getPeerUploader() const
    {
        return uploader;
    }

    //! Get the PeerManager
    PeerManager *getPeerManager()
    {
        return pman;
    }

    /*!
     * Send a chunk of data.
     * \param data The data
     * \param len The length
     * \return Number of bytes written
     */
    Uint32 sendData(const Uint8 *data, Uint32 len);

    /*!
     * Reads data from the peer.
     * \param buf The buffer to store the data
     * \param len The maximum number of bytes to read
     * \return The number of bytes read
     */
    Uint32 readData(Uint8 *buf, Uint32 len);

    //! Get the number of bytes available to read.
    Uint32 bytesAvailable() const;

    /*!
     * Close the peers connection.
     */
    void closeConnection();

    /*!
     * Kill the Peer.
     */
    void kill() override;

    //! Get the time in milliseconds since the last time a piece was received.
    Uint32 getTimeSinceLastPiece() const;

    //! Get the time the peer connection was established.
    const QTime &getConnectTime() const
    {
        return connect_time;
    }

    /*!
     * Get the percentual amount of data available from peer.
     */
    float percentAvailable() const;

    //! Set the ACA score
    void setACAScore(double s);

    bt::Uint32 averageDownloadSpeed() const override;

    //! Choke the peer
    void choke();

    /*!
     *  Emit the port packet signal.
     */
    void emitPortPacket();

    /*!
     * Emit the pex signal
     */
    void emitPex(const QByteArray &data, int ip_version);

    //! Disable or enable pex
    void setPexEnabled(bool on);

    /*!
     * Emit the metadataDownloaded signal
     */
    void emitMetadataDownloaded(const QByteArray &data);

    //! Send an extended protocol handshake
    void sendExtProtHandshake(Uint16 port, Uint32 metadata_size, bool partial_seed);

    /*!
     * Set the peer's group IDs for traffic
     * \param up_gid The upload gid
     * \param down_gid The download gid
     */
    void setGroupIDs(Uint32 up_gid, Uint32 down_gid);

    //! Enable or disable hostname resolving
    static void setResolveHostnames(bool on);

    //! Check if the peer has wanted chunks
    bool hasWantedChunks(const BitSet &wanted_chunks) const;

    /*!
     * Send a choke packet.
     */
    void sendChoke();

    /*!
     * Send an unchoke packet.
     */
    void sendUnchoke();

    /*!
     * Sends an unchoke message but doesn't update the am_choked field so KT still thinks
     * it is choked (and will not upload to it), this is to punish snubbers.
     */
    void sendEvilUnchoke();

    /*!
     * Send an interested packet.
     */
    void sendInterested();

    /*!
     * Send a not interested packet.
     */
    void sendNotInterested();

    /*!
     * Send a request for data.
     * \param r The Request
     */
    void sendRequest(const Request &r);

    /*!
     * Cancel a request.
     * \param r The Request
     */
    void sendCancel(const Request &r);

    /*!
     * Send a reject for a request
     * \param r The Request
     */
    void sendReject(const Request &r);

    /*!
     * Send a have packet.
     * \param index
     */
    void sendHave(Uint32 index);

    /*!
     * Send an allowed fast packet
     * \param index
     */
    void sendAllowedFast(Uint32 index);

    /*!
     * Send a chunk of data.
     * \param index Index of chunk
     * \param begin Offset into chunk
     * \param len Length of data
     * \param ch The Chunk
     * \return true If we satisfy the request, false otherwise
     */
    bool sendChunk(Uint32 index, Uint32 begin, Uint32 len, Chunk *ch);

    /*!
     * Send a BitSet. The BitSet indicates which chunks we have.
     * \param bs The BitSet
     */
    void sendBitSet(const BitSet &bs);

    /*!
     * Send a port message
     * \param port The port
     */
    void sendPort(Uint16 port);

    //! Send a have all message
    void sendHaveAll();

    //! Send a have none message
    void sendHaveNone();

    /*!
     * Send a suggest piece packet
     * \param index Index of the chunk
     */
    void sendSuggestPiece(Uint32 index);

    //! Send an extended protocol message
    void sendExtProtMsg(Uint8 id, const QByteArray &data);

    /*!
     * Clear all pending piece uploads we are not in the progress of sending.
     */
    void clearPendingPieceUploads();

    void chunkAllowed(Uint32 chunk) override;
    void handlePacket(const bt::Uint8 *packet, bt::Uint32 size) override;

    typedef QSharedPointer<Peer> Ptr;
    typedef QWeakPointer<Peer> WPtr;

private Q_SLOTS:
    void resolved(const QString &hinfo);

private:
    void handleChoke(Uint32 len);
    void handleUnchoke(Uint32 len);
    void handleInterested(Uint32 len);
    void handleNotInterested(Uint32 len);
    void handleHave(const Uint8 *packet, Uint32 len);
    void handleHaveAll(Uint32 len);
    void handleHaveNone(Uint32 len);
    void handleBitField(const Uint8 *packet, Uint32 len);
    void handleRequest(const Uint8 *packet, Uint32 len);
    void handlePiece(const Uint8 *packet, Uint32 len);
    void handleCancel(const Uint8 *packet, Uint32 len);
    void handleReject(const Uint8 *packet, Uint32 len);
    void handlePort(const Uint8 *packet, Uint32 len);
    void handleExtendedPacket(const Uint8 *packet, Uint32 size);
    void handleExtendedHandshake(const Uint8 *packet, Uint32 size);

Q_SIGNALS:
    /*!
     *  Emitted when metadata has been downloaded from the Peer
     */
    void metadataDownloaded(const QByteArray &data);

private:
    mse::EncryptedPacketSocket::Ptr sock;
    ConnectionLimit::Token::Ptr token;

    Timer stalled_timer;

    Uint32 id;

    Timer snub_timer;
    PacketReader *preader;
    PeerDownloader *downloader;
    PeerUploader *uploader;

    QTime connect_time;
    bool pex_allowed;
    PeerManager *pman;
    PtrMap<Uint32, PeerProtocolExtension> extensions;
    Uint32 ut_pex_id;

    Uint64 bytes_downloaded_since_unchoke;

    static bool resolve_hostname;

    friend class PeerDownloader;
};
}

#endif
