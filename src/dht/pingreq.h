/***************************************************************************
 *   Copyright (C) 2012 by Joris Guisson                                   *
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

#ifndef DHT_PINGREQ_H
#define DHT_PINGREQ_H

#include "rpcmsg.h"

namespace dht
{

	/**
	 * Ping request message in the DHT protocol
	 */
	class KTORRENT_EXPORT PingReq : public RPCMsg
	{
	public:
		PingReq();
		PingReq(const Key & id);
		virtual ~PingReq();
		
		virtual void apply(DHT* dh_table);
		virtual void print();
		virtual void encode(QByteArray & arr) const;
		
		typedef QSharedPointer<PingReq> Ptr;
	};

}

#endif // DHT_PINGREQ_H
