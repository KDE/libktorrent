/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "getpeersrsp.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
GetPeersRsp::GetPeersRsp()
    : RPCMsg(QByteArray(), dht::GET_PEERS, dht::RSP_MSG, QByteArray())
{
}

GetPeersRsp::GetPeersRsp(const QByteArray &mtid, const Key &id, const QByteArray &token)
    : RPCMsg(mtid, dht::GET_PEERS, dht::RSP_MSG, id)
    , token(token)
{
}

GetPeersRsp::GetPeersRsp(const QByteArray &mtid, const Key &id, const DBItemList &values, const QByteArray &token)
    : RPCMsg(mtid, dht::GET_PEERS, dht::RSP_MSG, id)
    , token(token)
    , items(values)
{
}

GetPeersRsp::~GetPeersRsp()
{
}

void GetPeersRsp::apply(dht::DHT *dh_table)
{
    dh_table->response(*this);
}

void GetPeersRsp::print()
{
    Out(SYS_DHT | LOG_DEBUG) << u"RSP: %1 %2 : get_peers(%3)"_s.arg(mtid[0]).arg(id.toString(), nodes.size() > 0 ? u"nodes"_s : u"values"_s) << endl;
}

void GetPeersRsp::encode(QByteArray &arr) const
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

            // must cast data() to (const Uint8*) to call right write() overload
            enc.write(QByteArrayLiteral("token"));
            enc.write((const Uint8 *)token.data(), token.size());

            if (items.size() > 0) {
                enc.write(QByteArrayLiteral("values"));
                enc.beginList();
                DBItemList::const_iterator i = items.begin();
                while (i != items.end()) {
                    const DBItem &item = *i;
                    Uint8 tmp[18];
                    Uint32 b = item.pack(tmp);
                    enc.write(tmp, b);
                    ++i;
                }
                enc.end();
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

void GetPeersRsp::parse(BDictNode *dict)
{
    dht::RPCMsg::parse(dict);
    BDictNode *args = dict->getDict(RSP);
    if (!args)
        throw bt::Error(u"Invalid response, arguments missing"_s);

    token = args->getByteArray("token").left(MAX_TOKEN_SIZE);

    BListNode *vals = args->getList("values");
    if (vals) {
        for (Uint32 i = 0; i < vals->getNumChildren(); i++) {
            QByteArray d = vals->getByteArray(i);
            if (d.length() == 6) { // IPv4
                Uint16 port = bt::ReadUint16((const Uint8 *)d.data(), 4);
                Uint32 ip = bt::ReadUint32((const Uint8 *)d.data(), 0);
                net::Address addr(QHostAddress(ip), port);
                items.append(DBItem(addr));
            } else if (d.length() == 18) { // IPv6
                Uint16 port = bt::ReadUint16((const Uint8 *)d.data(), 16);
                Q_IPV6ADDR ip;
                memcpy(ip.c, d.data(), 16);
                net::Address addr(QHostAddress(ip), port);
                items.append(DBItem(addr));
            }
        }
    }

    if (args->getValue("nodes") || args->getList("nodes6")) {
        BValueNode *v = args->getValue("nodes");
        if (v)
            nodes = v->data().toByteArray();

        v = args->getValue("nodes6");
        if (v)
            nodes6 = v->data().toByteArray();
    }
}
}
