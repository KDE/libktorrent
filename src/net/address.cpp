/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "address.h"
#include <sys/socket.h>
#include <util/log.h>

using namespace bt;

namespace net
{
Address::Address()
    : port_number(0)
{
}

Address::Address(quint32 ip4Addr, Uint16 port)
    : QHostAddress(ip4Addr)
    , port_number(port)
{
}

Address::Address(quint8 *ip6Addr, Uint16 port)
    : QHostAddress(ip6Addr)
    , port_number(port)
{
}

Address::Address(const Q_IPV6ADDR &ip6Addr, Uint16 port)
    : QHostAddress(ip6Addr)
    , port_number(port)
{
}

Address::Address(const struct sockaddr_storage *ss)
    : port_number(0)
{
    if (ss->ss_family == AF_INET) {
        setAddress((const struct sockaddr *)ss);
        port_number = ntohs(((const struct sockaddr_in *)ss)->sin_port);
    } else if (ss->ss_family == AF_INET6) {
        setAddress((const struct sockaddr *)ss);
        port_number = ntohs(((const struct sockaddr_in6 *)ss)->sin6_port);
        if (isIPv4Mapped()) {
            Q_IPV6ADDR ipv6 = toIPv6Address();
            quint32 ip = ipv6[12] << 24 | ipv6[13] << 16 | ipv6[14] << 8 | ipv6[15];
            setAddress(ip);
        }
    } else
        Out(SYS_GEN | LOG_DEBUG) << "Unknown address family" << endl;
}

Address::Address(const QString &host, Uint16 port)
    : QHostAddress(host)
    , port_number(port)
{
}

Address::Address(const Address &addr)
    : QHostAddress(addr)
    , port_number(addr.port_number)
{
}

Address::Address(const QHostAddress &addr, Uint16 port)
    : QHostAddress(addr)
    , port_number(port)
{
}

Address::~Address()
{
}

void Address::toSocketAddress(sockaddr_storage *ss, int &length) const
{
    if (protocol() == QAbstractSocket::IPv4Protocol) {
        struct sockaddr_in *addr = (struct sockaddr_in *)ss;
        memset(addr, 0, sizeof(struct sockaddr_in));
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port_number);
        addr->sin_addr.s_addr = htonl(toIPv4Address());
        length = sizeof(struct sockaddr_in);
    } else {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)ss;
        memset(addr, 0, sizeof(struct sockaddr_in6));
        addr->sin6_family = AF_INET6;
        addr->sin6_port = htons(port_number);
        memcpy(&addr->sin6_addr, toIPv6Address().c, 16);
        length = sizeof(struct sockaddr_in6);
    }
}

bool Address::operator==(const net::Address &other) const
{
    return QHostAddress::operator==(other) && port_number == other.port();
}

bool Address::operator<(const net::Address &other) const
{
    if (ipVersion() != other.ipVersion()) {
        return ipVersion() < other.ipVersion();
    } else if (other.ipVersion() == 4) {
        return toIPv4Address() == other.toIPv4Address() ? port_number < other.port_number : toIPv4Address() < other.toIPv4Address();
    } else { // IPv6
        if (QHostAddress::operator==(other))
            return port_number < other.port_number;
        else {
            Q_IPV6ADDR a = toIPv6Address();
            Q_IPV6ADDR b = other.toIPv6Address();
            for (int i = 0; i < 16; i++) {
                if (a[i] < b[i])
                    return true;
                else if (a[i] > b[i])
                    return false;
            }
            return false;
        }
    }
}

Address &Address::operator=(const net::Address &other)
{
    QHostAddress::operator=(other);
    port_number = other.port();
    return *this;
}

Address &Address::operator=(const struct sockaddr_storage &ss)
{
    if (ss.ss_family == AF_INET) {
        setAddress((const struct sockaddr *)&ss);
        port_number = ntohs(((const struct sockaddr_in *)&ss)->sin_port);
    } else if (ss.ss_family == AF_INET6) {
        setAddress((const struct sockaddr *)&ss);
        port_number = ntohs(((const struct sockaddr_in6 *)&ss)->sin6_port);
        if (isIPv4Mapped()) {
            Q_IPV6ADDR ipv6 = toIPv6Address();
            quint32 ip = ipv6[12] << 24 | ipv6[13] << 16 | ipv6[14] << 8 | ipv6[15];
            setAddress(ip);
        }
    }

    return *this;
}

bool Address::isIPv4Mapped() const
{
    Q_IPV6ADDR addr = toIPv6Address();
    for (int i = 0; i < 10; i++)
        if (addr[i] != 0)
            return false;

    return addr[10] == 0xff && addr[11] == 0xff;
}

Address Address::convertIPv4Mapped() const
{
    if (isIPv4Mapped()) {
        Q_IPV6ADDR ipv6 = toIPv6Address();
        quint32 ip = ipv6[12] << 24 | ipv6[13] << 16 | ipv6[14] << 8 | ipv6[15];
        return net::Address(ip, port());
    }

    return net::Address(*this);
}
}
