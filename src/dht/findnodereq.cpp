/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "findnodereq.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
FindNodeReq::FindNodeReq()
    : RPCMsg(QByteArray(), FIND_NODE, REQ_MSG, Key())
{
}

FindNodeReq::FindNodeReq(const Key &id, const Key &target)
    : RPCMsg(QByteArray(), FIND_NODE, REQ_MSG, id)
    , target(target)
{
}

FindNodeReq::~FindNodeReq()
{
}

void FindNodeReq::apply(dht::DHT *dh_table)
{
    dh_table->findNode(*this);
}

void FindNodeReq::print()
{
    Out(SYS_DHT | LOG_NOTICE) << QString("REQ: %1 %2 : find_node %3").arg(mtid[0]).arg(id.toString()).arg(target.toString()) << endl;
}

void FindNodeReq::encode(QByteArray &arr) const
{
    BEncoder enc(new BEncoderBufferOutput(arr));
    enc.beginDict();
    {
        enc.write(ARG);
        enc.beginDict();
        {
            enc.write(QByteArrayLiteral("id"));
            enc.write(id.getData(), 20);
            enc.write(QByteArrayLiteral("target"));
            enc.write(target.getData(), 20);
        }
        enc.end();
        enc.write(REQ);
        enc.write(QByteArrayLiteral("find_node"));
        enc.write(TID);
        enc.write(mtid);
        enc.write(TYP);
        enc.write(REQ);
    }
    enc.end();
}

void FindNodeReq::parse(BDictNode *dict)
{
    dht::RPCMsg::parse(dict);
    BDictNode *args = dict->getDict(ARG);
    if (!args)
        throw bt::Error("Invalid request, arguments missing");

    target = Key(args->getByteArray("target"));
    BListNode *ln = args->getList("want");
    if (ln) {
        for (bt::Uint32 i = 0; i < ln->getNumChildren(); i++)
            want.append(ln->getString(i, nullptr));
    }
}

bool FindNodeReq::wants(int ip_version) const
{
    return want.contains(QString("n%1").arg(ip_version));
}

}
