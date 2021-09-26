/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
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

#ifndef BT_SUPERSEEDER_H
#define BT_SUPERSEEDER_H

#include <QMap>
#include <QSet>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
class ChunkCounter;
class PeerInterface;
class BitSet;

/**
    Class which manages superseeding. Superseeding is a way to achieve much higher seeding
    efficiences, thereby allowing a peer to use much less bandwidth to get a torrent seeded.
    @see http://bittorrent.org/beps/bep_0016.html
*/
class KTORRENT_EXPORT SuperSeeder
{
public:
    /**
        Constructor.
        @param num_chunks The number of chunks
    */
    SuperSeeder(Uint32 num_chunks);
    virtual ~SuperSeeder();

    /**
        A HAVE message was sent by a Peer
        @param peer The Peer
        @param chunk The chunk
    */
    void have(PeerInterface *peer, bt::Uint32 chunk);

    /**
        A HAVE_ALL message was sent by a Peer
        @param peer The Peer
    */
    void haveAll(PeerInterface *peer);

    /**
        A BITSET message was sent by a Peer
        @param peer The Peer
    */
    void bitset(PeerInterface *peer, const BitSet &bs);

    /**
        A Peer has been added
        @param peer The Peer
    */
    void peerAdded(PeerInterface *peer);

    /**
        A Peer has been removed
        @param peer The Peer
    */
    void peerRemoved(PeerInterface *peer);

    /**
        Dump the status of the SuperSeeder for debugging purposes.
    */
    void dump();

private:
    void sendChunk(PeerInterface *peer);

private:
    ChunkCounter *chunk_counter;
    QMultiMap<bt::Uint32, bt::PeerInterface *> active_chunks;
    QMap<bt::PeerInterface *, bt::Uint32> active_peers;
    Uint32 num_seeders;

    typedef QMultiMap<bt::Uint32, bt::PeerInterface *>::iterator ActiveChunkItr;
    typedef QMap<bt::PeerInterface *, bt::Uint32>::iterator ActivePeerItr;
};

}

#endif // BT_SUPERSEEDER_H
