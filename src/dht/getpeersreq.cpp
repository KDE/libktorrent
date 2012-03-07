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

#include "getpeersreq.h"
#include <util/log.h>
#include <util/error.h>
#include <bcodec/bnode.h>
#include <bcodec/bencoder.h>
#include "dht.h"


using namespace bt;

namespace dht
{
	GetPeersReq::GetPeersReq()
			: RPCMsg(QByteArray(), GET_PEERS, REQ_MSG, Key())
	{
	}

	GetPeersReq::GetPeersReq(const Key & id, const Key & info_hash)
			: RPCMsg(QByteArray(), GET_PEERS, REQ_MSG, id), info_hash(info_hash)
	{}

	GetPeersReq::~GetPeersReq()
	{}

	void GetPeersReq::apply(dht::DHT* dh_table)
	{
		dh_table->getPeers(*this);
	}

	void GetPeersReq::print()
	{
		Out(SYS_DHT | LOG_DEBUG) << QString("REQ: %1 %2 : get_peers %3")
		.arg(mtid[0]).arg(id.toString()).arg(info_hash.toString()) << endl;
	}

	void GetPeersReq::encode(QByteArray & arr) const
	{
		BEncoder enc(new BEncoderBufferOutput(arr));
		enc.beginDict();
		{
			enc.write(ARG); enc.beginDict();
			{
				enc.write(QString("id")); enc.write(id.getData(), 20);
				enc.write(QString("info_hash")); enc.write(info_hash.getData(), 20);
			}
			enc.end();
			enc.write(REQ); enc.write(QString("get_peers"));
			enc.write(TID); enc.write(mtid);
			enc.write(TYP); enc.write(REQ);
		}
		enc.end();
	}

	void GetPeersReq::parse(BDictNode* dict)
	{
		dht::RPCMsg::parse(dict);
		BDictNode* args = dict->getDict(ARG);
		if (!args)
			throw bt::Error("Invalid request, arguments missing");

		info_hash = Key(args->getByteArray("info_hash"));
	}

}

