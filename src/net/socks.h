/***************************************************************************
 *   Copyright (C) 2007 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
 *   ivasic@gmail.com                                                      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#ifndef NETSOCKS_H
#define NETSOCKS_H

#include <net/address.h>
#include <net/addressresolver.h>
#include <ktorrent_export.h>
#include <mse/encryptedpacketsocket.h>

namespace net
{
/**
 * @author Joris Guisson
 *
 * Class which handles the SOCKSv5 protocol
*/
class KTORRENT_EXPORT Socks : public QObject
{
    Q_OBJECT
public:
    enum State {
        IDLE,
        CONNECTING_TO_SERVER,
        CONNECTING_TO_HOST,
        CONNECTED,
        FAILED,
    };

    enum SetupState {
        NONE,
        AUTH_REQUEST_SENT,
        USERNAME_AND_PASSWORD_SENT,
        CONNECT_REQUEST_SENT,
    };
    Socks(mse::EncryptedPacketSocket::Ptr sock, const Address & dest);
    ~Socks() override;

    /// Setup a socks connection, return the current state
    State setup();

    /**
     * The socket is ready to write (used to determine if we are connected to the server)
     * @return The current state
     */
    State onReadyToWrite();

    /**
     * There is data available on the socked
     * @return The current state
     */
    State onReadyToRead();

    /// Is socks enabled
    static bool enabled()
    {
        return socks_enabled;
    }

    /// Enable or disable socks
    static void setSocksEnabled(bool on)
    {
        socks_enabled = on;
    }

    /// Set the socks server address
    static void setSocksServerAddress(const QString & host, bt::Uint16 port);

    /// Set the socks version (4 or 5)
    static void setSocksVersion(int version)
    {
        socks_version = version;
    }

    /**
     * Set the SOCKSv5 Username and password
     * @param username The username
     * @param password The password
     */
    static void setSocksAuthentication(const QString & username, const QString & password);

private:
    State sendAuthRequest();
    void sendConnectRequest();
    void sendUsernamePassword();
    State handleAuthReply();
    State handleUsernamePasswordReply();
    State handleConnectReply();

private Q_SLOTS:
    void resolved(net::AddressResolver* ar);

private:
    mse::EncryptedPacketSocket::Ptr sock;
    Address dest;
    State state;
    SetupState internal_state;
    int version;

    static net::Address socks_server_addr;
    static bool socks_server_addr_resolved;
    static QString socks_server_host;
    static bt::Uint16 socks_server_port;
    static bool socks_enabled;
    static int socks_version;
    static QString socks_username;
    static QString socks_password;
};

}

#endif
