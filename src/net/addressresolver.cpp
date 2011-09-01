/***************************************************************************
 *   Copyright (C) 2011 by Joris Guisson                                   *
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


#include "addressresolver.h"

namespace net
{

	AddressResolver::AddressResolver(const QString & host, bt::Uint16 port, QObject* parent, const char* slot) :
		QObject(parent),
		lookup_id(-1),
		succesfull(false)
	{
		result.setPort(port);
		lookup_id = QHostInfo::lookupHost(host, this, SLOT(hostResolved(QHostInfo)));
		ongoing = true;
		connect(this, SIGNAL(resolved(net::AddressResolver*)), parent, slot);
	}

	AddressResolver::~AddressResolver()
	{
		if (ongoing)
			QHostInfo::abortHostLookup(lookup_id);
	}
	
	void AddressResolver::hostResolved(const QHostInfo& res)
	{
		ongoing = false;
		succesfull = res.error() == QHostInfo::NoError && res.addresses().count() > 0;
		if (!succesfull)
		{
			resolved(this);
		}
		else
		{
			result = net::Address(res.addresses().first(), result.port());
			resolved(this);
		}
		
		deleteLater();
	}
	
	void AddressResolver::resolve(const QString& host, Uint16 port, QObject* parent, const char* slot)
	{
		new AddressResolver(host, port, parent, slot);
	}

	Address AddressResolver::resolve(const QString& host, Uint16 port)
	{
		QHostInfo info = QHostInfo::fromName(host);
		if (info.error() == QHostInfo::NoError && info.addresses().size() > 0)
			return net::Address(info.addresses().first(), port);
		else
			return net::Address();
	}


}
