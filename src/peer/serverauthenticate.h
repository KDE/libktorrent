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
#ifndef BTSERVERAUTHENTICATE_H
#define BTSERVERAUTHENTICATE_H

#include "authenticatebase.h"

namespace bt
{
/**
 * @author Joris Guisson
 *
 * Handles the authentication of incoming connections on the Server.
 * Once the authentication is finished, the socket gets handed over
 * to the right PeerManager.
 */
class ServerAuthenticate : public AuthenticateBase
{
    Q_OBJECT
public:
    ServerAuthenticate(mse::EncryptedPacketSocket::Ptr sock);
    ~ServerAuthenticate() override;

    static bool isFirewalled();
    static void setFirewalled(bool Firewalled);

protected:
    void onFinish(bool succes) override;
    void handshakeReceived(bool full) override;

private:
    static bool s_firewalled;
};

}

inline bool bt::ServerAuthenticate::isFirewalled()
{
    return s_firewalled;
}

inline void bt::ServerAuthenticate::setFirewalled(bool Firewalled)
{
    s_firewalled = Firewalled;
}

#endif
