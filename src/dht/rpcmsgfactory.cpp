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

#include "rpcmsgfactory.h"
#include <util/log.h>
#include <util/error.h>
#include <util/functions.h>
#include <bcodec/bnode.h>
#include "rpcserver.h"
#include "dht.h"
#include "errmsg.h"
#include "pingreq.h"
#include "findnodereq.h"
#include "announcereq.h"
#include "pingrsp.h"
#include "findnodersp.h"
#include "getpeersrsp.h"
#include "announcersp.h"

using namespace bt;

namespace dht
{

	RPCMsgFactory::RPCMsgFactory()
	{

	}

	RPCMsgFactory::~RPCMsgFactory()
	{

	}
	
	RPCMsg::Ptr RPCMsgFactory::buildRequest(BDictNode* dict)
	{
		BDictNode* args = dict->getDict(ARG);
		if (!args)
			throw bt::Error("Invalid request, arguments missing");
		
		RPCMsg::Ptr msg;
		QString str = dict->getString(REQ, 0);
		if (str == "ping")
		{
			msg = RPCMsg::Ptr(new PingReq());
			msg->parse(dict);
			return msg;
		}
		else if (str == "find_node")
		{
			msg = RPCMsg::Ptr(new FindNodeReq());
			msg->parse(dict);
			return msg;
		}
		else if (str == "get_peers")
		{
			msg = RPCMsg::Ptr(new GetPeersReq());
			msg->parse(dict);
			return msg;
		}
		else if (str == "announce_peer")
		{
			msg = RPCMsg::Ptr(new AnnounceReq());
			msg->parse(dict);
			return msg;
		}
		else if (str == "vote")
		{
			// Some µTorrent extension to rate torrents, just ignore
			return msg;
		}
		else
			throw bt::Error(QString("Invalid request type %1").arg(str));
	}
	

	RPCMsg::Ptr RPCMsgFactory::buildResponse(BDictNode* dict, dht::RPCMethodResolver* method_resolver)
	{
		BDictNode* args = dict->getDict(RSP);
		if (!args)
			throw bt::Error("Arguments missing for DHT response");
		
		QByteArray mtid = dict->getByteArray(TID);
		// check for empty byte arrays should prevent 144416
		if (mtid.size() == 0)
			throw bt::Error("Empty transaction ID in DHT response");
		
		RPCMsg::Ptr msg;
		
		// find the call
		Method method = method_resolver->findMethod(mtid);
		switch (method)
		{
			case PING:
				msg = RPCMsg::Ptr(new PingRsp());
				msg->parse(dict);
				break;
			case FIND_NODE:
				msg = RPCMsg::Ptr(new FindNodeRsp());
				msg->parse(dict);
				break;
			case GET_PEERS:
				msg = RPCMsg::Ptr(new GetPeersRsp());
				msg->parse(dict);
				break;
			case ANNOUNCE_PEER:
				msg = RPCMsg::Ptr(new AnnounceRsp());
				msg->parse(dict);
				break;
			case NONE:
			default:
				throw bt::Error(QString("Unknown DHT rpc call (transaction id = %1)").arg(mtid[0]));
		}
		
		return msg;
	}


	RPCMsg::Ptr RPCMsgFactory::build(bt::BDictNode* dict, RPCMethodResolver* method_resolver)
	{
		QString t = dict->getString(TYP, 0);
		if (t == REQ)
		{
			return buildRequest(dict);
		}
		else if (t == RSP)
		{
			return buildResponse(dict, method_resolver);
		}
		else if (t == ERR_DHT)
		{
			RPCMsg::Ptr msg(new ErrMsg());
			msg->parse(dict);
			return msg;
		}
		else
			throw bt::Error(QString("Unknown message type %1").arg(t));
	}

}
