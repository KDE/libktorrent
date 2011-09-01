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

#include "address.h"
#include <arpa/inet.h>


namespace net
{

	Address::Address() : port_number(0)
	{
	}
	
	Address::Address(quint32 ip4Addr, Uint16 port): QHostAddress(ip4Addr), port_number(port)
	{
	}

	Address::Address(quint8* ip6Addr, Uint16 port): QHostAddress(ip6Addr), port_number(port)
	{
	}

	Address::Address(const Q_IPV6ADDR& ip6Addr, Uint16 port): QHostAddress(ip6Addr), port_number(port)
	{
	}
	
	Address::Address(const struct sockaddr_storage* ss) : port_number(0)
	{
		if (ss->ss_family == AF_INET)
		{
			setAddress((const struct sockaddr*)ss);
			port_number = ntohs(((const struct sockaddr_in*)ss)->sin_port);
		}
		else if (ss->ss_family == AF_INET6)
		{
			char tmp[100];
			inet_ntop(AF_INET, &((const struct sockaddr_in6*)ss)->sin6_addr, tmp, 100);
			setAddress(QString(tmp));
			port_number = ntohs(((const struct sockaddr_in6*)ss)->sin6_port);
		}
	}

	
	Address::Address(const QString & host,Uint16 port) : QHostAddress(host), port_number(port)
	{
	}
	
	Address::Address(const Address & addr) : QHostAddress(addr), port_number(addr.port_number)
	{
	}
	
	Address::Address(const QHostAddress & addr, Uint16 port) : QHostAddress(addr), port_number(port)
	{}
	
	Address:: ~Address()
	{}

	void Address::toSocketAddress(sockaddr_storage* ss, int& length) const
	{
		if (protocol() == QAbstractSocket::IPv4Protocol)
		{
			struct sockaddr_in* addr = (struct sockaddr_in*)ss;
			memset(addr, 0, sizeof(struct sockaddr_in));
			addr->sin_family = AF_INET;
			addr->sin_port = htons(port_number);
			inet_pton(AF_INET, toString().toLocal8Bit().data(), &addr->sin_addr);
			length = sizeof(struct sockaddr_in);
		}
		else
		{
			struct sockaddr_in6* addr = (struct sockaddr_in6*)ss;
			memset(addr, 0, sizeof(struct sockaddr_in6));
			addr->sin6_family = AF_INET;
			addr->sin6_port = htons(port_number);
			inet_pton(AF_INET6, toString().toLocal8Bit().data(), &addr->sin6_addr);
			length = sizeof(struct sockaddr_in6);
		}
	}

	bool Address::operator == (const net::Address& other) const
	{
		return QHostAddress::operator == (other) && port_number == other.port();
	}
	
	bool Address::operator < (const net::Address& other) const
	{
		if (ipVersion() == 4 && other.ipVersion() == 6)
			return true;
		else if (other.ipVersion() == 4 && ipVersion() == 6)
			return false;
		else if (other.ipVersion() == 4)
		{
			return toIPv4Address() == other.toIPv4Address() ? 
					port_number < other.port_number : 
					toIPv4Address() < other.toIPv4Address();
		}
		else // IPv6
		{
			if (QHostAddress::operator == (other))
				return port_number < other.port_number;
			else
			{
				Q_IPV6ADDR a = toIPv6Address();
				Q_IPV6ADDR b = other.toIPv6Address();
				for (int i = 0; i < 16; i++)
					if (a[i] < b[i])
						return true;
					
				return false;
			}
		}
	}

	Address& Address::operator = (const net::Address& other)
	{
		QHostAddress::operator = (other);
		port_number = other.port();
		return *this;
	}

}
