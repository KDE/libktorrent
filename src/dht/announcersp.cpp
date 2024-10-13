/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "announcersp.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
AnnounceRsp::AnnounceRsp()
    : RPCMsg(QByteArray(), ANNOUNCE_PEER, RSP_MSG, Key())
{
}

AnnounceRsp::AnnounceRsp(const QByteArray &mtid, const Key &id)
    : RPCMsg(mtid, ANNOUNCE_PEER, RSP_MSG, id)
{
}

AnnounceRsp::~AnnounceRsp()
{
}

void AnnounceRsp::apply(dht::DHT *dh_table)
{
    dh_table->response(*this);
}

void AnnounceRsp::print()
{
    Out(SYS_DHT | LOG_DEBUG) << u"RSP: %1 %2 : announce_peer"_s.arg(mtid[0]).arg(id.toString()) << endl;
}

void AnnounceRsp::encode(QByteArray &arr) const
{
    BEncoder enc(new BEncoderBufferOutput(arr));
    enc.beginDict();
    {
        enc.write(RSP);
        enc.beginDict();
        {
            enc.write(QByteArrayLiteral("id"));
            enc.write(id.getData(), 20);
        }
        enc.end();
        enc.write(TID);
        enc.write(mtid);
        enc.write(TYP);
        enc.write(RSP);
    }
    enc.end();
}

void AnnounceRsp::parse(BDictNode *dict)
{
    dht::RPCMsg::parse(dict);
    BDictNode *args = dict->getDict(RSP);
    if (!args)
        throw bt::Error(u"Invalid response, arguments missing"_s);
}

}
