/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "findnodersp.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
FindNodeRsp::FindNodeRsp()
    : RPCMsg(QByteArray(), FIND_NODE, RSP_MSG, Key())
{
}

FindNodeRsp::FindNodeRsp(const QByteArray &mtid, const Key &id)
    : RPCMsg(mtid, FIND_NODE, RSP_MSG, id)
{
}

FindNodeRsp::~FindNodeRsp()
{
}

void FindNodeRsp::apply(dht::DHT *dh_table)
{
    dh_table->response(*this);
}

void FindNodeRsp::print()
{
    Out(SYS_DHT | LOG_DEBUG) << u"RSP: %1 %2 : find_node"_s.arg(mtid[0]).arg(id.toString()) << endl;
}

void FindNodeRsp::encode(QByteArray &arr) const
{
    BEncoder enc(new BEncoderBufferOutput(arr));
    enc.beginDict();
    {
        enc.write(RSP);
        enc.beginDict();
        {
            enc.write(QByteArrayLiteral("id"));
            enc.write(id.getData(), 20);
            if (nodes.size() > 0) {
                enc.write(QByteArrayLiteral("nodes"));
                enc.write(nodes);
            }

            if (nodes6.size() > 0) {
                enc.write(QByteArrayLiteral("nodes6"));
                enc.write(nodes6);
            }
        }
        enc.end();
        enc.write(TID);
        enc.write(mtid);
        enc.write(TYP);
        enc.write(RSP);
    }
    enc.end();
}

void FindNodeRsp::parse(BDictNode *dict)
{
    dht::RPCMsg::parse(dict);
    BDictNode *args = dict->getDict(RSP);
    if (!args)
        throw bt::Error(u"Invalid response, arguments missing"_s);

    if (!args->getValue("nodes") && !args->getList("nodes6"))
        throw bt::Error(u"Missing nodes or nodes6 parameter"_s);

    BValueNode *v = args->getValue("nodes");
    if (v)
        nodes = v->data().toByteArray();

    v = args->getValue("nodes6");
    if (v)
        nodes6 = v->data().toByteArray();
}

}
