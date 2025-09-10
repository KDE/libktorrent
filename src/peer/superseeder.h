/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_SUPERSEEDER_H
#define BT_SUPERSEEDER_H

#include <QMap>
#include <QSet>
#include <ktorrent_export.h>
#include <util/constants.h>

#include <memory>

namespace bt
{
class ChunkCounter;
class PeerInterface;
class BitSet;

/*!
    \headerfile peer/superseeder.h
    \brief Manages the superseeding extension.

    Superseeding is a way to achieve much higher seeding efficiences, thereby allowing a peer to use much less bandwidth to get a torrent seeded.
    \sa http://bittorrent.org/beps/bep_0016.html
*/
class KTORRENT_EXPORT SuperSeeder
{
public:
    /*!
        Constructor.
        \param num_chunks The number of chunks
    */
    SuperSeeder(Uint32 num_chunks);
    virtual ~SuperSeeder();

    /*!
        A HAVE message was sent by a Peer
        \param peer The Peer
        \param chunk The chunk
    */
    void have(PeerInterface *peer, bt::Uint32 chunk);

    /*!
        A HAVE_ALL message was sent by a Peer
        \param peer The Peer
    */
    void haveAll(PeerInterface *peer);

    /*!
        A BITSET message was sent by a Peer
        \param peer The Peer
        \param bs The bitset
    */
    void bitset(PeerInterface *peer, const BitSet &bs);

    /*!
        A Peer has been added
        \param peer The Peer
    */
    void peerAdded(PeerInterface *peer);

    /*!
        A Peer has been removed
        \param peer The Peer
    */
    void peerRemoved(PeerInterface *peer);

    /*!
        Dump the status of the SuperSeeder for debugging purposes.
    */
    void dump();

private:
    void sendChunk(PeerInterface *peer);

private:
    std::unique_ptr<ChunkCounter> chunk_counter;
    QMultiMap<bt::Uint32, bt::PeerInterface *> active_chunks;
    QMap<bt::PeerInterface *, bt::Uint32> active_peers;
    Uint32 num_seeders;

    using ActiveChunkItr = QMultiMap<bt::Uint32, bt::PeerInterface *>::iterator;
    using ActivePeerItr = QMap<bt::PeerInterface *, bt::Uint32>::iterator;
};

}

#endif // BT_SUPERSEEDER_H
