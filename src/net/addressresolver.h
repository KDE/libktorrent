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


#ifndef NET_ADDRESSRESOLVER_H
#define NET_ADDRESSRESOLVER_H

#include <QHostInfo>
#include <ktorrent_export.h>
#include <net/address.h>

namespace net 
{

	/**
	 * Resolves hostnames into net::Address objects.
	 * This class will clean itself up, after it is done using deleteLater.
	 **/
	class KTORRENT_EXPORT AddressResolver : public QObject
	{
		Q_OBJECT
	public:
		/**
		 * Constructor, initializer the lookup.
		 * @param host Hostname
		 * @param port Port number
		 * @param parent Parent
		 * @param slot Slot of parent to connect to
		 **/
		AddressResolver(const QString & host, bt::Uint16 port, QObject* parent, const char* slot);
		~AddressResolver() override;
		
		/// Dit the resolver succeed ?
		bool succeeded() const {return succesfull;}
		
		/// Get the resulting address
		const net::Address & address() const {return result;} 
		
		/**
		 * Convenience method to resolve a hostname.
		 * @param host Hostname                   
		 * @param port Port number
		 * @param parent Parent
		 * @param slot Slot of parent to connect to
		 * @return void
		 **/
		static void resolve(const QString & host, bt::Uint16 port, QObject* parent, const char* slot);
		
		/**
		 * Synchronous resolve
		 * @param host Hostname
		 * @param port Port number
		 * @return :Address
		 **/
		static net::Address resolve(const QString & host, bt::Uint16 port);
		
	Q_SIGNALS:
		/**
		 * Emitted when hostname lookup succeeded
		 * @param ar This AddressResolver
		 **/
		void resolved(net::AddressResolver* ar);
		
	private Q_SLOTS:
		void hostResolved(const QHostInfo & res);
		
	private:
		int lookup_id;
		net::Address result;
		bool succesfull;
		bool ongoing;
	};

}

#endif // NET_ADDRESSRESOLVER_H
