/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETADDRESS_H
#define NETADDRESS_H

#include <QHostAddress>
#include <ktorrent_export.h>
#include <util/constants.h>

#ifndef Q_OS_WIN
#include <netinet/in.h>
#else
#include <Winsock2.h>
#endif

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

    /**
     * Convert to a struct sockaddr_storage
     *
     * @param ss the returned sockaddr_storage
     * @param length the size of the returned address
     * @param as_ipv6 if the address is IPv4 this parameter determines whether to return an IPv4 address or an IPv4-mapped IPv6 address. If the address is
     * already IPv6 then this parameter is unused.
     */
    void toSocketAddress(struct sockaddr_storage *ss, int &length, bool as_ipv6 = false) const;

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
