/*
    SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETSOCKS_H
#define NETSOCKS_H

#include <ktorrent_export.h>
#include <mse/encryptedpacketsocket.h>
#include <net/address.h>
#include <net/addressresolver.h>

namespace net
{
/*!
 * \author Joris Guisson
 *
 * \brief Handles the SOCKSv5 protocol.
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

    Socks(mse::EncryptedPacketSocket *sock, const Address &dest);
    ~Socks() override;

    //! Setup a socks connection, return the current state
    State setup();

    /*!
     * The socket is ready to write (used to determine if we are connected to the server)
     * \return The current state
     */
    State onReadyToWrite();

    /*!
     * There is data available on the socked
     * \return The current state
     */
    State onReadyToRead();

    //! Is socks enabled
    static bool enabled()
    {
        return socks_enabled;
    }

    //! Enable or disable socks
    static void setSocksEnabled(bool on)
    {
        socks_enabled = on;
    }

    //! Set the socks server address
    static void setSocksServerAddress(const QString &host, bt::Uint16 port);

    //! Set the socks version (4 or 5)
    static void setSocksVersion(int version)
    {
        socks_version = version;
    }

    /*!
     * Set the SOCKSv5 Username and password
     * \param username The username
     * \param password The password
     */
    static void setSocksAuthentication(const QString &username, const QString &password);

private:
    State sendAuthRequest();
    void sendConnectRequest();
    void sendUsernamePassword();
    State handleAuthReply();
    State handleUsernamePasswordReply();
    State handleConnectReply();

private Q_SLOTS:
    void resolved(net::AddressResolver *ar);

private:
    enum SetupState {
        NONE,
        AUTH_REQUEST_SENT,
        USERNAME_AND_PASSWORD_SENT,
        CONNECT_REQUEST_SENT,
    };

    mse::EncryptedPacketSocket *sock;
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
