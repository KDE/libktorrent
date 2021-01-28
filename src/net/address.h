/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
#ifndef NETADDRESS_H
#define NETADDRESS_H

#include <QHostAddress>
#include <ktorrent_export.h>
#include <netinet/in.h>
#include <util/constants.h>

namespace net
{
using bt::Uint16;
using bt::Uint32;

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Network address, contains an IP address and a port number.
 * This supports both IPv4 and IPv6 addresses.
 */
class KTORRENT_EXPORT Address : public QHostAddress
{
public:
    Address();
    Address(quint32 ip4Addr, Uint16 port);
    Address(quint8 *ip6Addr, Uint16 port);
    Address(const Q_IPV6ADDR &ip6Addr, Uint16 port);
    Address(const struct sockaddr_storage *ss);
    Address(const QString &host, Uint16 port);
    Address(const QHostAddress &addr, Uint16 port);
    Address(const Address &addr);
    virtual ~Address();

    /// Get the port number
    Uint16 port() const
    {
        return port_number;
    }

    /// Set the port number
    void setPort(Uint16 p)
    {
        port_number = p;
    }

    /// Convert to a struct sockaddr_storage
    void toSocketAddress(struct sockaddr_storage *ss, int &length) const;

    /// Return the IP protocol version (4 or 6)
    int ipVersion() const
    {
        return protocol() == QAbstractSocket::IPv4Protocol ? 4 : 6;
    }

    /// Equality operator
    bool operator==(const net::Address &other) const;

    /// Less then operator for putting net::Address in a map
    bool operator<(const net::Address &other) const;

    /// Assignment operator
    Address &operator=(const net::Address &other);

    /// Assignment operator
    Address &operator=(const struct sockaddr_storage &ss);

    /// Is this a IPv4 mapped address into the IPv6 address space
    bool isIPv4Mapped() const;

    /// Convert an IPv4 mapped IPv6 address to an IPv4 address
    Address convertIPv4Mapped() const;

private:
    Uint16 port_number;
};

}

#endif
