/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
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

#ifndef UTP_UTPSERVER_H
#define UTP_UTPSERVER_H

#include <QThread>
#include <interfaces/serverinterface.h>
#include <net/address.h>
#include <net/poll.h>
#include <utp/connection.h>

namespace utp
{
/**
    Implements the UTP server. It listens for UTP packets and manages all connections.
*/
class KTORRENT_EXPORT UTPServer : public bt::ServerInterface, public Transmitter
{
    Q_OBJECT
public:
    UTPServer(QObject *parent = nullptr);
    ~UTPServer() override;

    /// Enabled creating sockets (tests need to have this disabled)
    void setCreateSockets(bool on);

    bool changePort(bt::Uint16 port) override;

    /// Send a packet to some host
    bool sendTo(Connection::Ptr conn, const PacketBuffer &packet) override;

    /// Setup a connection to a remote address
    Connection::WPtr connectTo(const net::Address &addr);

    /// Get the last accepted connection (Note: for unittest purposes)
    Connection::WPtr acceptedConnection();

    /// Start the UTP server
    void start();

    /// Stop the UTP server
    void stop();

    /// Prepare the server for polling
    void preparePolling(net::Poll *p, net::Poll::Mode mode, Connection::Ptr &conn);

    /// Set the TOS byte
    void setTOS(bt::Uint8 type_of_service);

    /// Thread has been started
    void threadStarted();

    /**
        Handle newly connected sockets, it starts authentication on them.
        This needs to be called from the main thread.
    */
    void handlePendingConnections();

protected:
    virtual void handlePacket(bt::Buffer::Ptr buffer, const net::Address &addr);
    void stateChanged(Connection::Ptr conn, bool readable, bool writeable) override;
    void closed(Connection::Ptr conn) override;
    void customEvent(QEvent *ev) override;

Q_SIGNALS:
    void handlePendingConnectionsDelayed();
    /// Emitted when a connection is accepted if creating sockets is disabled
    void accepted();

private:
    void cleanup();
    void checkTimeouts();

private:
    class Private;
    Private *d;
};

}

#endif // UTP_UTPSERVER_H
