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

#ifndef DHT_GETPEERSRSP_H
#define DHT_GETPEERSRSP_H


#include "rpcmsg.h"
#include "packednodecontainer.h"

namespace dht
{

	/**
	 * GetPeers response message
	 */
	class KTORRENT_EXPORT GetPeersRsp : public RPCMsg, public PackedNodeContainer
	{
	public:
		GetPeersRsp();
		GetPeersRsp(const QByteArray & mtid, const Key & id, const Key & token);
		GetPeersRsp(const QByteArray & mtid, const Key & id, const DBItemList & values, const Key & token);
		virtual ~GetPeersRsp();

		virtual void apply(DHT* dh_table);
		virtual void print();
		virtual void encode(QByteArray & arr) const;
		virtual void parse(bt::BDictNode* dict);

		const DBItemList & getItemList() const {return items;}
		const Key & getToken() const {return token;}
		bool containsNodes() const {return nodes.size() > 0 || nodes2.count() > 0;}
		bool containsValues() const {return nodes.size() == 0;}

		typedef QSharedPointer<GetPeersRsp> Ptr;
	private:
		Key token;
		DBItemList items;
	};

}

#endif // DHT_GETPEERSRSP_H
