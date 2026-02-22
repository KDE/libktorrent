/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "announcereq.h"
#include "dht.h"
#include <bcodec/bencoder.h>
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace dht
{
AnnounceReq::AnnounceReq()
{
    method = dht::ANNOUNCE_PEER;
}

AnnounceReq::AnnounceReq(const Key &id, const Key &info_hash, Uint16 port, const QByteArray &token)
    : GetPeersReq(id, info_hash)
    , port(port)
    , token(token)
{
    method = dht::ANNOUNCE_PEER;
}

AnnounceReq::~AnnounceReq()
{
}

void AnnounceReq::apply(dht::DHT *dh_table)
{
    dh_table->announce(*this);
}

void AnnounceReq::print()
{
    Out(SYS_DHT | LOG_DEBUG)
        << u"REQ: %1 %2 : announce_peer %3 %4 %5"_s.arg(mtid[0]).arg(id.toString(), info_hash.toString()).arg(port).arg(QString::fromLatin1(token.toHex()))
        << endl;
}

void AnnounceReq::encode(QByteArray &arr) const
{
    BEncoder enc(std::make_unique<BEncoderBufferOutput>(arr));
    enc.beginDict();
    {
        enc.write(ARG);
        enc.beginDict();
        {
            enc.write("id", id);
            enc.write("info_hash", info_hash);
            enc.write("port", (Uint32)port);
            enc.write("token", token);
        }
        enc.end();
        enc.write(REQ, "announce_peer");
        enc.write(TID, mtid);
        enc.write(TYP, REQ);
    }
    enc.end();
}

void AnnounceReq::parse(BDictNode *dict)
{
    dht::GetPeersReq::parse(dict);
    BDictNode *args = dict->getDict(ARG);
    if (!args) {
        throw bt::Error(u"Invalid request, arguments missing"_s);
    }

    info_hash = Key(args->getByteArray("info_hash"));
    port = args->getInt("port");
    token = args->getByteArray("token").left(MAX_TOKEN_SIZE);
}
}
