/***************************************************************************
 *   Copyright (C) 2012 by                                                 *
 *   Joris Guisson <joris.guisson@gmail.com>                               *
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

#ifndef DHT_PACKEDNODECONTAINER_H
#define DHT_PACKEDNODECONTAINER_H

#include <QList>
#include <QByteArray>

namespace dht
{

	/**
	 * Packed node container utilitiy class.
	 * Stores both nodes and nodes2 parameters of some DHT messages.
	 */
	class PackedNodeContainer
	{
	public:
		PackedNodeContainer();
		virtual ~PackedNodeContainer();
		
		typedef QList<QByteArray>::const_iterator CItr;
		
		CItr begin() const {return nodes2.begin();}
		CItr end() const {return nodes2.end();}
		
		/// Add a single node to the nodes or nodes2 parameter depending on it's size
		void addNode(const QByteArray & a);
		
		/// Set the nodes parameter
		void setNodes(const QByteArray & n) {nodes = n;}
		
		/// Get the nodes parameter
		const QByteArray & getNodes() const {return nodes;}
	protected:
		QByteArray nodes;
		QList<QByteArray> nodes2;
	};
	
}

#endif // DHT_PACKEDNODECONTAINER_H
