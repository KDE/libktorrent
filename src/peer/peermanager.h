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
#ifndef BTPEERMANAGER_H
#define BTPEERMANAGER_H


#include <interfaces/peersource.h>
#include <ktorrent_export.h>
#include <peer/superseeder.h>
#include <peer/peerconnector.h>
#include <mse/streamsocket.h>

namespace KNetwork
{
	class KResolverResults;
}

namespace bt
{
	class Peer;
	class PeerID;
	class Piece;
	class Torrent;
	class Authenticate;
	class ChunkCounter;
	class PieceDownloader;

	using KNetwork::KResolverResults;
	
	const Uint32 MAX_SIMULTANIOUS_AUTHS = 20;
	
	/// Base class for handling pieces
	class KTORRENT_EXPORT PieceHandler
	{
	public:
		virtual ~PieceHandler() {}
		
		virtual void pieceReceived(const Piece & p) = 0;
	};

	/**
	 * @author Joris Guisson
	 * @brief Manages all the Peers
	 * 
	 * This class manages all Peer objects.
	 * It can also open connections to other peers.
	 */
	class KTORRENT_EXPORT PeerManager : public QObject, public SuperSeederClient
	{
		Q_OBJECT
	public:
		/**
		 * Constructor.
		 * @param tor The Torrent
		 */
		PeerManager(Torrent & tor);
		virtual ~PeerManager();
		

		/**
		 * Check for new connections, update down and upload speed of each Peer.
		 * Initiate new connections. 
		 */
		void update();
		
		/**
		 * Pause the peer connections
		 */
		void pause();
		
		/**
		 * Unpause the peer connections 
		 */
		void unpause();
		
		/**
		 * Remove dead peers.
		 * @return The number of dead ones removed
		 */
		Uint32 clearDeadPeers();

		/**
		 * Get a list of all peers.
		 * @return A QList of Peer's
		 */
		QList<Peer*> getPeers() const;

		/**
		 * Find a Peer based on it's ID
		 * @param peer_id The ID
		 * @return A Peer or 0, if nothing could be found
		 */
		Peer* findPeer(Uint32 peer_id);
		
		/**
		 * Find a Peer based on it's PieceDownloader
		 * @param pd The PieceDownloader
		 * @return The matching Peer or 0 if none can be found
		 */
		Peer* findPeer(PieceDownloader* pd);
		
		void setWantedChunks(const BitSet & bs);

		/**
		 * Try to connect to some peers
		 */
		void connectToPeers();
		
		/**
		 * Close all Peer connections.
		 */
		void closeAllConnections();
		
		/**
		 * Start listening to incoming requests.
		 * @param superseed Set to true to get superseeding
		 */
		void start(bool superseed);
		
		/**
		 * Stop listening to incoming requests.
		 */
		void stop();
		
		/**
		 * Kill all peers who appear to be stale
		 */
		void killStalePeers();
		
		/// Get the number of connected peers
		Uint32 getNumConnectedPeers() const;
		
		/// Get the number of pending peers we are attempting to connect to
		Uint32 getNumPending() const;
		
		static void setMaxConnections(Uint32 max);
		static Uint32 getMaxConnections() {return max_connections;}
		
		static void setMaxTotalConnections(Uint32 max);
		static Uint32 getMaxTotalConnections() {return max_total_connections;}
		
		static Uint32 getTotalConnections() {return total_connections;}
		
		/// Is the peer manager started
		bool isStarted() const;

		/// Get the Torrent
		const Torrent & getTorrent() const;
		
		/// Get the combined upload rate of all peers in bytes per sec
		Uint32 uploadRate() const;

		/**
		 * A new connection is ready for this PeerManager.
		 * @param sock The socket
		 * @param peer_id The Peer's ID
		 * @param support What extensions the peer supports
		 */
		void newConnection(mse::StreamSocket::Ptr sock,const PeerID & peer_id,Uint32 support);

		/**
		 * Add a potential peer
		 * @param pp The PotentialPeer
		 */
		void addPotentialPeer(const PotentialPeer & pp);
		
		/**
		 * Kills all connections to seeders. 
		 * This is used when torrent download gets finished 
		 * and we should drop all connections to seeders
		 */
		void killSeeders();
		
		/**
		 * Kills all peers that are not interested for a long time.
		 * This should be used when torrent is seeding ONLY.
		 */
		void killUninterested();

		/// Get a BitSet of all available chunks
		const BitSet & getAvailableChunksBitSet() const;
		
		/// Get the chunk counter.
		ChunkCounter & getChunkCounter();
	
		/// Are we connected to a Peer given it's PeerID ?
		bool connectedTo(const PeerID & peer_id);	
		
		/**
		 * A peer has authenticated.
		 * @param auth The Authenticate object
		 * @param pcon The PeerConnector
		 * @param ok Whether or not the attempt was succesfull
		 */
		void peerAuthenticated(Authenticate* auth,PeerConnector::WPtr pcon,bool ok);
		
		/**
		 * Save the IP's and port numbers of all peers.
		 */
		void savePeerList(const QString & file);
		
		/**
		 * Load the peer list again and add them to the potential peers
		 */
		void loadPeerList(const QString & file);
		
		class PeerVisitor
		{
		public:
			virtual ~PeerVisitor() {}
			
			/// Called for each Peer
			virtual void visit(const Peer* p) = 0;
		};
		
		/// Visit all peers
		void visit(PeerVisitor & visitor);
		
		/// Is PEX eanbled
		bool isPexEnabled() const;
		
		/// Enable or disable PEX
		void setPexEnabled(bool on);
		
		/// Set the group IDs of each peer
		void setGroupIDs(Uint32 up,Uint32 down);
		
		/// Have message received by a peer
		void have(Peer* p,Uint32 index);
		
		/// Bitset received by a peer
		void bitSetReceived(Peer* p,const BitSet & bs);
		
		/// Rerun the choker
		void rerunChoker();
		
		/// A PEX message was received
		void pex(const QByteArray & arr);
		
		/// A port packet was received
		void portPacketReceived(const QString & ip,Uint16 port);
		
		/// A Piece was received
		void pieceReceived(const Piece & p);
		
		/// Set the piece handler
		void setPieceHandler(PieceHandler* ph);
		
		/// Enable or disable super seeding
		void setSuperSeeding(bool on,const BitSet & chunks);
		
		/// Send a have message to all peers
		void sendHave(Uint32 index);
		
	public slots:
		/**
		 * A PeerSource, has new potential peers.
		 * @param ps The PeerSource
		 */
		void peerSourceReady(PeerSource* ps);
		
	private:
		virtual void allowChunk(PeerInterface* peer, Uint32 chunk);

	private slots:
		void onResolverResults(KNetwork::KResolverResults res);
		
	signals:
		void newPeer(Peer* p);
		void peerKilled(Peer* p);
		
	private:
		class Private;
		Private* d;
		
		static Uint32 max_connections;
		static Uint32 max_total_connections;
		static Uint32 total_connections;
	};

}

#endif
