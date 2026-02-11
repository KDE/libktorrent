/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEERMANAGER_H
#define BTPEERMANAGER_H

#include <interfaces/peersource.h>
#include <ktorrent_export.h>
#include <mse/encryptedpacketsocket.h>
#include <peer/peer.h>
#include <peer/peerconnector.h>
#include <peer/superseeder.h>

#include <memory>

namespace bt
{
class PeerID;
class Piece;
class Torrent;
class Authenticate;
class ChunkCounter;
class PieceDownloader;
class ConnectionLimit;

const Uint32 MAX_SIMULTANIOUS_AUTHS = 20;

/*!
 * \brief Interface that is notified whenever a piece is received.
 */
class KTORRENT_EXPORT PieceHandler
{
public:
    virtual ~PieceHandler()
    {
    }

    virtual void pieceReceived(const Piece &p) = 0;
};

/*!
 * \author Joris Guisson
 * \brief Manages all the Peers for a given torrent.
 *
 * This class manages all Peer objects.
 * It can also open connections to other peers.
 */
class KTORRENT_EXPORT PeerManager : public QObject
{
    Q_OBJECT
public:
    /*!
     * Constructor.
     * \param tor The Torrent
     */
    PeerManager(Torrent &tor);
    ~PeerManager() override;

    //! Get the connection limits
    static ConnectionLimit &connectionLimits();

    /*!
     * Check for new connections, update down and upload speed of each Peer.
     * Initiate new connections.
     */
    void update();

    /*!
     * Pause the peer connections
     */
    void pause();

    /*!
     * Unpause the peer connections
     */
    void unpause();

    /*!
     * Get a list of all peers.
     * \return A QList of Peer's
     */
    [[nodiscard]] QList<Peer *> getPeers() const;

    /*!
     * Find a Peer based on it's ID
     * \param peer_id The ID
     * \return A Peer or nullptr, if nothing could be found
     */
    Peer *findPeer(Uint32 peer_id);

    /*!
     * Find a Peer based on it's PieceDownloader
     * \param pd The PieceDownloader
     * \return The matching Peer or nullptr if none can be found
     */
    Peer *findPeer(PieceDownloader *pd);

    void setWantedChunks(const BitSet &bs);

    /*!
     * Close all Peer connections.
     */
    void closeAllConnections();

    /*!
     * Start listening to incoming requests.
     * \param superseed Set to true to get superseeding
     */
    void start(bool superseed);

    /*!
     * Stop listening to incoming requests.
     */
    void stop();

    /*!
     * Kill all peers who appear to be stale
     */
    void killStalePeers();

    //! Get the number of connected peers
    [[nodiscard]] Uint32 getNumConnectedPeers() const;

    //! Get the number of connected seeders
    [[nodiscard]] Uint32 getNumConnectedSeeders() const;

    //! Get the number of connected leechers
    [[nodiscard]] Uint32 getNumConnectedLeechers() const;

    //! Get the number of pending peers we are attempting to connect to
    [[nodiscard]] Uint32 getNumPending() const;

    //! Is the peer manager started
    [[nodiscard]] bool isStarted() const;

    //! Get the Torrent
    [[nodiscard]] const Torrent &getTorrent() const;

    //! Get the combined upload rate of all peers in bytes per sec
    [[nodiscard]] Uint32 uploadRate() const;

    /*!
     * A new connection is ready for this PeerManager.
     * \param sock The socket
     * \param peer_id The Peer's ID
     * \param support What extensions the peer supports
     */
    void newConnection(std::unique_ptr<mse::EncryptedPacketSocket> sock, const PeerID &peer_id, Uint32 support);

    /*!
     * Add a potential peer
     * \param addr The peers' address
     * \param local Is it a peer on the local network
     **/
    void addPotentialPeer(const net::Address &addr, bool local);

    /*!
     * Kills all connections to seeders.
     * This is used when torrent download gets finished
     * and we should drop all connections to seeders
     */
    void killSeeders();

    /*!
     * Kills all peers that are not interested for a long time.
     * This should be used when torrent is seeding ONLY.
     */
    void killUninterested();

    //! Get a BitSet of all available chunks
    [[nodiscard]] const BitSet &getAvailableChunksBitSet() const;

    //! Get the chunk counter.
    ChunkCounter &getChunkCounter();

    //! Are we connected to a Peer given it's PeerID ?
    bool connectedTo(const PeerID &peer_id);

    /*!
     * A peer has authenticated.
     * \param auth The Authenticate object
     * \param pcon The PeerConnector
     * \param ok Whether or not the attempt was succesfull
     * \param token The ConnectionLimit::Token
     */
    void peerAuthenticated(Authenticate *auth, PeerConnector::WPtr pcon, bool ok, std::unique_ptr<ConnectionLimit::Token> token);

    /*!
     * Save the IP's and port numbers of all peers.
     */
    void savePeerList(const QString &file);

    /*!
     * Load the peer list again and add them to the potential peers
     */
    void loadPeerList(const QString &file);

    /*!
     * \brief Implements the visitor pattern for all Peers owned by the PeerManager.
     */
    class PeerVisitor
    {
    public:
        virtual ~PeerVisitor()
        {
        }

        //! Called for each Peer
        virtual void visit(const Peer *p) = 0;
    };

    //! Visit all peers
    void visit(PeerVisitor &visitor);

    //! Is PEX enabled
    [[nodiscard]] bool isPexEnabled() const;

    //! Enable or disable PEX
    void setPexEnabled(bool on);

    //! Set the group IDs of each peer
    void setGroupIDs(Uint32 up, Uint32 down);

    //! Have message received by a peer
    void have(Peer *p, Uint32 index);

    //! Bitset received by a peer
    void bitSetReceived(Peer *p, const BitSet &bs);

    //! Rerun the choker
    void rerunChoker();

    //! Does the choker need to run again
    [[nodiscard]] bool chokerNeedsToRun() const;

    //! A PEX message was received
    void pex(const QByteArray &arr, int ip_version);

    //! A port packet was received
    void portPacketReceived(const QString &ip, Uint16 port);

    //! A Piece was received
    void pieceReceived(const Piece &p);

    //! Set the piece handler
    void setPieceHandler(PieceHandler *ph);

    //! Enable or disable super seeding
    void setSuperSeeding(bool on, const BitSet &chunks);

    //! Send a have message to all peers
    void sendHave(Uint32 index);

    //! Set if we are a partial seed or not
    void setPartialSeed(bool partial_seed);

    //! Are we a partial seed
    [[nodiscard]] bool isPartialSeed() const;

public Q_SLOTS:
    /*!
     * A PeerSource, has new potential peers.
     * \param ps The PeerSource
     */
    void peerSourceReady(bt::PeerSource *ps);

Q_SIGNALS:
    void newPeer(bt::Peer *p);
    void peerKilled(bt::Peer *p);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}

#endif
