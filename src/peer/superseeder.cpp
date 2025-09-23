/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "superseeder.h"
#include "chunkcounter.h"

#include <QRandomGenerator>

#include <interfaces/peerinterface.h>
#include <util/log.h>

namespace bt
{
SuperSeeder::SuperSeeder(Uint32 num_chunks)
    : chunk_counter(new ChunkCounter(num_chunks))
    , num_seeders(0)
{
}

SuperSeeder::~SuperSeeder()
{
}

void SuperSeeder::have(PeerInterface *peer, Uint32 chunk)
{
    chunk_counter->inc(chunk);
    if (peer->getBitSet().allOn()) // it is possible the peer has become a seeder
        num_seeders++;

    QList<PeerInterface *> peers;

    ActiveChunkItr i = active_chunks.find(chunk);
    while (i != active_chunks.end() && i.key() == chunk) {
        // Somebody else has an active chunk we sent to p2
        PeerInterface *p2 = i.value();
        if (peer != p2) {
            active_peers.remove(p2);
            i = active_chunks.erase(i);
            // p2 has probably been a good boy, he can download another chunk from us
            peers.append(p2);
        } else
            ++i;
    }

    for (PeerInterface *p : std::as_const(peers)) {
        sendChunk(p);
    }
}

void SuperSeeder::haveAll(PeerInterface *peer)
{
    // Lets just ignore seeders
    if (active_peers.contains(peer)) {
        Uint32 chunk = active_peers[peer];
        active_chunks.remove(chunk, peer);
        active_peers.remove(peer);
    }

    num_seeders++;
}

void SuperSeeder::bitset(PeerInterface *peer, const bt::BitSet &bs)
{
    if (bs.allOn()) {
        haveAll(peer);
        return;
    }

    // Call have for each chunk the peer has
    for (Uint32 i = 0; i < bs.getNumBits(); i++) {
        if (bs.get(i))
            have(peer, i);
    }
}

void SuperSeeder::peerAdded(PeerInterface *peer)
{
    if (peer->getBitSet().allOn()) {
        num_seeders++;
    } else {
        chunk_counter->incBitSet(peer->getBitSet());
        sendChunk(peer);
    }
}

void SuperSeeder::peerRemoved(PeerInterface *peer)
{
    // remove the peer
    if (active_peers.contains(peer)) {
        Uint32 chunk = active_peers[peer];
        active_chunks.remove(chunk, peer);
        active_peers.remove(peer);
    }

    // decrease num_seeders if the peer is a seeder
    if (peer->getBitSet().allOn() && num_seeders > 0)
        num_seeders--;

    chunk_counter->decBitSet(peer->getBitSet());
}

void SuperSeeder::sendChunk(PeerInterface *peer)
{
    if (active_peers.contains(peer))
        return;

    const BitSet &bs = peer->getBitSet();
    if (bs.allOn())
        return;

    // Use random chunk to start searching for a potential chunk we can send
    Uint32 num_chunks = chunk_counter->getNumChunks();
    Uint32 start = QRandomGenerator::global()->bounded(num_chunks);
    Uint32 chunk = (start + 1) % num_chunks;
    Uint32 alternative = num_chunks;
    Uint32 alternative_num_owners = std::numeric_limits<Uint32>::max();
    while (chunk != start) {
        chunk = (chunk + 1) % num_chunks;
        if (bs.get(chunk))
            continue;

        // Search for a chunk which no downloader has, or has been sent.
        // Otherwise choose the rarest chunk
        const Uint32 num_chunk_owners = chunk_counter->get(chunk);
        if (num_chunk_owners == num_seeders && !active_chunks.contains(chunk)) {
            peer->chunkAllowed(chunk);
            active_chunks.insert(chunk, peer);
            active_peers[peer] = chunk;
            return;
        } else if (num_chunk_owners < alternative_num_owners) {
            alternative = chunk;
        }
    }

    if (alternative < num_chunks) {
        peer->chunkAllowed(alternative);
        active_chunks.insert(alternative, peer);
        active_peers[peer] = alternative;
    }
}

void SuperSeeder::dump()
{
    Out(SYS_GEN | LOG_DEBUG) << "Active chunks: " << endl;
    ActiveChunkItr i = active_chunks.begin();
    while (i != active_chunks.end()) {
        Out(SYS_GEN | LOG_DEBUG) << "Chunk " << i.key() << " : " << i.value()->getPeerID().toString() << endl;
        ++i;
    }

    Out(SYS_GEN | LOG_DEBUG) << "Active peers: " << endl;
    ActivePeerItr j = active_peers.begin();
    while (j != active_peers.end()) {
        Out(SYS_GEN | LOG_DEBUG) << "Peer " << j.key()->getPeerID().toString() << " : " << j.value() << endl;
        ++j;
    }
}

}
