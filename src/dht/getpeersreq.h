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

#ifndef DHT_GETPEERSREQ_H
#define DHT_GETPEERSREQ_H


#include "rpcmsg.h"

namespace dht
{

	/**
	 * GetPeers request in the DHT protocol
	 */
	class KTORRENT_EXPORT GetPeersReq : public RPCMsg
	{
	public:
		GetPeersReq();
		GetPeersReq(const Key & id,const Key & info_hash);
		virtual ~GetPeersReq();
		
		const Key & getInfoHash() const {return info_hash;}
		virtual void apply(DHT* dh_table);
		virtual void print();
		virtual void encode(QByteArray & arr) const;
		virtual void parse(bt::BDictNode* dict);
		
		typedef QSharedPointer<GetPeersReq> Ptr;
	protected:
		Key info_hash;
	};
	
}

#endif // DHT_GETPEERSREQ_H
