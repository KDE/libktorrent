/*
    SPDX-FileCopyrightText: 2012 by
    Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "getpeersreq.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
GetPeersReq::GetPeersReq()
    : RPCMsg(QByteArray(), GET_PEERS, REQ_MSG, Key())
{
}

GetPeersReq::GetPeersReq(const Key &id, const Key &info_hash)
    : RPCMsg(QByteArray(), GET_PEERS, REQ_MSG, id)
    , info_hash(info_hash)
{
}

GetPeersReq::~GetPeersReq()
{
}

void GetPeersReq::apply(dht::DHT *dh_table)
{
    dh_table->getPeers(*this);
}

void GetPeersReq::print()
{
    Out(SYS_DHT | LOG_DEBUG) << QString("REQ: %1 %2 : get_peers %3").arg(mtid[0]).arg(id.toString()).arg(info_hash.toString()) << endl;
}

void GetPeersReq::encode(QByteArray &arr) const
{
    BEncoder enc(new BEncoderBufferOutput(arr));
    enc.beginDict();
    {
        enc.write(ARG);
        enc.beginDict();
        {
            enc.write(QByteArrayLiteral("id"));
            enc.write(id.getData(), 20);
            enc.write(QByteArrayLiteral("info_hash"));
            enc.write(info_hash.getData(), 20);
        }
        enc.end();
        enc.write(REQ);
        enc.write(QByteArrayLiteral("get_peers"));
        enc.write(TID);
        enc.write(mtid);
        enc.write(TYP);
        enc.write(REQ);
    }
    enc.end();
}

void GetPeersReq::parse(BDictNode *dict)
{
    dht::RPCMsg::parse(dict);
    BDictNode *args = dict->getDict(ARG);
    if (!args)
        throw bt::Error("Invalid request, arguments missing");

    info_hash = Key(args->getByteArray("info_hash"));
    BListNode *ln = args->getList("want");
    if (ln) {
        for (bt::Uint32 i = 0; i < ln->getNumChildren(); i++)
            want.append(ln->getString(i, nullptr));
    }
}

bool GetPeersReq::wants(int ip_version) const
{
    return want.contains(QString("n%1").arg(ip_version));
}

}
