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
#ifndef BTAUTHENTICATE_H
#define BTAUTHENTICATE_H

#include "authenticatebase.h"
#include <peer/peerconnector.h>
#include <peer/peerid.h>
#include <util/sha1hash.h>

namespace net
{
class Socks;
}

namespace bt
{
/**
 * @author Joris Guisson
 * @brief Authenicate a peer
 *
 * After we connect to a peer,
 * we need to authenticate the peer. This class handles this.
 */
class Authenticate : public AuthenticateBase
{
    Q_OBJECT
public:
    /**
     * Connect to a remote host first and authenicate it.
     * @param addr Address to connect to
     * @param proto Transport protocol to use
     * @param info_hash Info hash
     * @param peer_id Peer ID
     * @param pman PeerManager
     */
    Authenticate(const net::Address &addr, TransportProtocol proto, const SHA1Hash &info_hash, const PeerID &peer_id, PeerConnector::WPtr pcon);

    ~Authenticate() override;

    const PeerID &getPeerID() const
    {
        return peer_id;
    }

    /// See if the authentication is succesfull
    bool isSuccesfull() const
    {
        return succes;
    }

public Q_SLOTS:
    /// Stop the authentication
    void stop();

protected Q_SLOTS:
    void onReadyWrite() override;
    void onReadyRead() override;

protected:
    void onFinish(bool succes) override;
    void handshakeReceived(bool full) override;
    virtual void connected();

protected:
    SHA1Hash info_hash;
    PeerID our_peer_id, peer_id;
    net::Address addr;
    bool succes;
    PeerConnector::WPtr pcon;
    net::Socks *socks;
};
}

#endif
