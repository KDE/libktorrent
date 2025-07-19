/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "rpcmsgfactory.h"
#include "announcereq.h"
#include "announcersp.h"
#include "dht.h"
#include "errmsg.h"
#include "findnodereq.h"
#include "findnodersp.h"
#include "getpeersrsp.h"
#include "pingreq.h"
#include "pingrsp.h"
#include "rpcserver.h"
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
RPCMsgFactory::RPCMsgFactory()
{
}

RPCMsgFactory::~RPCMsgFactory()
{
}

RPCMsg::Ptr RPCMsgFactory::buildRequest(BDictNode *dict)
{
    BDictNode *args = dict->getDict(ARG);
    if (!args)
        throw bt::Error(u"Invalid request, arguments missing"_s);

    RPCMsg::Ptr msg;
    const auto str = dict->getByteArray(REQ);
    if (str == "ping") {
        msg = std::make_unique<PingReq>();
        msg->parse(dict);
        return msg;
    } else if (str == "find_node") {
        msg = std::make_unique<FindNodeReq>();
        msg->parse(dict);
        return msg;
    } else if (str == "get_peers") {
        msg = std::make_unique<GetPeersReq>();
        msg->parse(dict);
        return msg;
    } else if (str == "announce_peer") {
        msg = std::make_unique<AnnounceReq>();
        msg->parse(dict);
        return msg;
    } else if (str == "vote") {
        // Some µTorrent extension to rate torrents, just ignore
        return msg;
    } else
        throw bt::Error(u"Invalid request type %1"_s.arg(QLatin1StringView(str)));
}

RPCMsg::Ptr RPCMsgFactory::buildResponse(BDictNode *dict, dht::RPCMethodResolver *method_resolver)
{
    BDictNode *args = dict->getDict(RSP);
    if (!args)
        throw bt::Error(u"Arguments missing for DHT response"_s);

    QByteArray mtid = dict->getByteArray(TID);
    // check for empty byte arrays should prevent 144416
    if (mtid.size() == 0)
        throw bt::Error(u"Empty transaction ID in DHT response"_s);

    RPCMsg::Ptr msg;

    // find the call
    Method method = method_resolver->findMethod(mtid);
    switch (method) {
    case PING:
        msg = std::make_unique<PingRsp>();
        msg->parse(dict);
        break;
    case FIND_NODE:
        msg = std::make_unique<FindNodeRsp>();
        msg->parse(dict);
        break;
    case GET_PEERS:
        msg = std::make_unique<GetPeersRsp>();
        msg->parse(dict);
        break;
    case ANNOUNCE_PEER:
        msg = std::make_unique<AnnounceRsp>();
        msg->parse(dict);
        break;
    case NONE:
    default:
        throw bt::Error(u"Unknown DHT rpc call (transaction id = %1)"_s.arg(mtid[0]));
    }

    return msg;
}

RPCMsg::Ptr RPCMsgFactory::build(bt::BDictNode *dict, RPCMethodResolver *method_resolver)
{
    const auto t = dict->getByteArray(TYP);
    if (t == REQ) {
        return buildRequest(dict);
    } else if (t == RSP) {
        return buildResponse(dict, method_resolver);
    } else if (t == ERR_DHT) {
        auto msg = std::make_unique<ErrMsg>();
        msg->parse(dict);
        return msg;
    } else
        throw bt::Error(u"Unknown message type %1"_s.arg(QLatin1StringView(t)));
}

}
