/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
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
