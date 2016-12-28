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
#include "findnodereq.h"
#include <util/log.h>
#include <util/error.h>
#include <bcodec/bnode.h>
#include <bcodec/bencoder.h>
#include "dht.h"

using namespace bt;

namespace dht
{

	FindNodeReq::FindNodeReq() :
			RPCMsg(QByteArray(), FIND_NODE, REQ_MSG, Key())
	{
	}

	FindNodeReq::FindNodeReq(const Key & id, const Key & target) :
		RPCMsg(QByteArray(), FIND_NODE, REQ_MSG, id),
		target(target)
	{
	}

	FindNodeReq::~FindNodeReq()
	{
	}

	void FindNodeReq::apply(dht::DHT* dh_table)
	{
		dh_table->findNode(*this);
	}

	void FindNodeReq::print()
	{
		Out(SYS_DHT | LOG_NOTICE) << QString("REQ: %1 %2 : find_node %3")
		.arg(mtid[0]).arg(id.toString()).arg(target.toString()) << endl;
	}

	void FindNodeReq::encode(QByteArray & arr) const
	{
		BEncoder enc(new BEncoderBufferOutput(arr));
		enc.beginDict();
		{
			enc.write(ARG); enc.beginDict();
			{
				enc.write(QByteArrayLiteral("id")); enc.write(id.getData(), 20);
				enc.write(QByteArrayLiteral("target")); enc.write(target.getData(), 20);
			}
			enc.end();
			enc.write(REQ); enc.write(QByteArrayLiteral("find_node"));
			enc.write(TID); enc.write(mtid);
			enc.write(TYP); enc.write(REQ);
		}
		enc.end();
	}

	void FindNodeReq::parse(BDictNode* dict)
	{
		dht::RPCMsg::parse(dict);
		BDictNode* args = dict->getDict(ARG);
		if (!args)
			throw bt::Error("Invalid request, arguments missing");

		target = Key(args->getByteArray("target"));
		BListNode* ln = args->getList("want");
		if (ln)
		{
			for (bt::Uint32 i = 0; i < ln->getNumChildren(); i++)
				want.append(ln->getString(i, 0));
		}
	}

	bool FindNodeReq::wants(int ip_version) const
	{
		return want.contains(QString("n%1").arg(ip_version));
	}

}
