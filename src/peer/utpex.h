/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
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
#ifndef BTUTPEX_H
#define BTUTPEX_H

#include "peerprotocolextension.h"
#include <ktorrent_export.h>
#include <map>
#include <net/address.h>
#include <peer/peermanager.h>
#include <util/constants.h>

namespace bt
{
class Peer;
class BEncoder;

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Class which handles µTorrent's peer exchange
 */
class KTORRENT_EXPORT UTPex : public PeerProtocolExtension, public PeerManager::PeerVisitor
{
public:
    UTPex(Peer *peer, Uint32 id);
    ~UTPex() override;

    /**
     * Handle a PEX packet
     * @param packet The packet
     * @param size The size of the packet
     */
    void handlePacket(const Uint8 *packet, Uint32 size) override;

    /// Do we need to update PEX (should happen every minute)
    bool needsUpdate() const override;

    /// Send a new PEX packet to the Peer
    void update() override;

    /// Change the ID used in the extended packets
    void changeID(Uint32 nid)
    {
        id = nid;
    }

    /// Globally disable or enabled PEX
    static void setEnabled(bool on)
    {
        pex_enabled = on;
    }

    /// Is PEX enabled globally
    static bool isEnabled()
    {
        return pex_enabled;
    }

private:
    void encode(BEncoder &enc, const std::map<Uint32, net::Address> &ps);
    void encodeFlags(BEncoder &enc, const std::map<Uint32, Uint8> &flags);
    void visit(const bt::Peer::Ptr p) override;

private:
    std::map<Uint32, net::Address> peers;
    TimeStamp last_updated;
    static bool pex_enabled;

    std::map<Uint32, net::Address> added;
    std::map<Uint32, Uint8> flags;
    std::map<Uint32, net::Address> npeers;
};

}

#endif
