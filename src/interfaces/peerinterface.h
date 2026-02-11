/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEERINTERFACE_H
#define BTPEERINTERFACE_H

#include <ktorrent_export.h>
#include <peer/peerid.h>
#include <util/bitset.h>
#include <util/constants.h>

namespace bt
{
/*!
 * \author Joris Guisson
 * \brief Interface for a Peer.
 *
 * This is the interface for a Peer, it allows other classes to
 * get statistics about a Peer, and provides some basic functionality provided by a Peer.
 */
class KTORRENT_EXPORT PeerInterface
{
public:
    /*!
        Constructor, initialize the PeerID and the number of chunks
        \param peer_id The PeerID
        \param num_chunks The number of chunks
    */
    PeerInterface(const PeerID &peer_id, Uint32 num_chunks);
    virtual ~PeerInterface();

    /*!
     * \brief Information about the peer.
     */
    struct Stats {
        //! IP address of peer (dotted notation)
        QString ip_address;
        //! Host name of the peer
        QString hostname;
        //! The client (Azureus, BitComet, ...)
        QString client;
        //! Download rate (bytes/s)
        bt::Uint32 download_rate;
        //! Upload rate (bytes/s)
        bt::Uint32 upload_rate;
        //! Choked or not
        bool choked;
        //! Snubbed or not (i.e. we haven't received a piece for a minute)
        bool snubbed;
        //! Percentage of file which the peer has
        float perc_of_file;
        //! Does this peer support DHT
        bool dht_support;
        //! Amount of data uploaded
        bt::Uint64 bytes_uploaded;
        //! Amount of data downloaded
        bt::Uint64 bytes_downloaded;
        //! Advanced choke algorithm score
        double aca_score;
        //! Flag to indicate if this peer has an upload slot
        bool has_upload_slot;
        //! Is the peer interested
        bool interested;
        //! Am I interested in the peer
        bool am_interested;
        //! Whether or not this connection is encrypted
        bool encrypted;
        //! Number of upload requests queued
        bt::Uint32 num_up_requests;
        //! Number of outstanding download requests queued
        bt::Uint32 num_down_requests;
        //! Supports the fast extensions
        bool fast_extensions;
        //! Is this a peer on the local network
        bool local;
        //! Whether or not the peer supports the extension protocol
        bool extension_protocol;
        //! Max number of outstanding requests (reqq in extended protocol handshake)
        bt::Uint32 max_request_queue;
        //! Time the peer choked us
        TimeStamp time_choked;
        //! Time the peer unchoked us
        TimeStamp time_unchoked;
        //! The transport protocol used by the peer
        bt::TransportProtocol transport_protocol;
        //! Is this a partial seed
        bool partial_seed;

        //! Get the address of the peer (hostname if it is valid, IP otherwise)
        [[nodiscard]] QString address() const
        {
            return hostname.isEmpty() ? ip_address : hostname;
        }
    };

    //! Get the Peer's statistics
    const Stats &getStats() const
    {
        return stats;
    }

    /*!
        Kill the Peer, will ensure the PeerManager closes the connection, and cleans things up.
    */
    virtual void kill() = 0;

    //! See if the peer has been killed.
    bool isKilled() const
    {
        return killed;
    }

    /*!
        Get the average download speed since the last unchoke in bytes/sec
     */
    virtual bt::Uint32 averageDownloadSpeed() const = 0;

    //! Get the Peer's BitSet
    const BitSet &getBitSet() const
    {
        return pieces;
    }

    //! Get the Peer's ID
    const PeerID &getPeerID() const
    {
        return peer_id;
    }

    //! Is the Peer choked
    bool isChoked() const
    {
        return stats.choked;
    }

    //! Is the Peer interested
    bool isInterested() const
    {
        return stats.interested;
    }

    //! Are we interested in the Peer
    bool areWeInterested() const
    {
        return stats.am_interested;
    }

    //! Are we choked for the Peer
    bool areWeChoked() const
    {
        return !stats.has_upload_slot || paused;
    }

    //! See if the peer supports DHT
    bool isDHTSupported() const
    {
        return stats.dht_support;
    }

    //! Get the time when this Peer choked us
    TimeStamp getChokeTime() const
    {
        return stats.time_choked;
    }

    //! Get the time when this Peer unchoked us
    TimeStamp getUnchokeTime() const
    {
        return stats.time_unchoked;
    }

    //! See if the peer is a seeder.
    bool isSeeder() const
    {
        return pieces.allOn();
    }

    //! Peer is allowed to download chunk (used for superseeding)
    virtual void chunkAllowed(bt::Uint32 chunk) = 0;

    //! Handle a received packet
    virtual void handlePacket(const bt::Uint8 *packet, bt::Uint32 size) = 0;

protected:
    mutable PeerInterface::Stats stats;
    bool paused;
    bool killed;
    PeerID peer_id;
    BitSet pieces;
};

}

#endif
