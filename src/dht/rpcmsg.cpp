/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "rpcmsg.h"
#include <bcodec/bnode.h>
#include <util/error.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
RPCMsg::RPCMsg()
    : mtid(nullptr)
    , method(NONE)
    , type(INVALID)
{
}

RPCMsg::RPCMsg(const QByteArray &mtid, Method m, Type type, const Key &id)
    : mtid(mtid)
    , method(m)
    , type(type)
    , id(id)
{
}

RPCMsg::~RPCMsg()
{
}

void RPCMsg::parse(bt::BDictNode *dict)
{
    mtid = dict->getByteArray(TID);
    if (mtid.isEmpty())
        throw bt::Error(u"Invalid DHT transaction ID"_s);

    const auto t = dict->getByteArray(TYP);
    if (t == REQ) {
        type = REQ_MSG;
        BDictNode *args = dict->getDict(ARG);
        if (!args)
            return;

        id = Key(args->getByteArray("id"));
    } else if (t == RSP) {
        type = RSP_MSG;
        BDictNode *args = dict->getDict(RSP);
        if (!args)
            return;

        id = Key(args->getByteArray("id"));
    } else if (t == ERR_DHT)
        type = ERR_MSG;
    else
        throw bt::Error(u"Unknown message type %1"_s.arg(QLatin1StringView(t)));
}
}
