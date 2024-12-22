/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "errmsg.h"
#include "dht.h"
#include <bcodec/bnode.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
ErrMsg::ErrMsg()
{
}

ErrMsg::ErrMsg(const QByteArray &mtid, const Key &id, const QString &msg)
    : RPCMsg(mtid, NONE, ERR_MSG, id)
    , msg(msg)
{
}

ErrMsg::~ErrMsg()
{
}

void ErrMsg::apply(dht::DHT *dh_table)
{
    dh_table->error(*this);
}

void ErrMsg::print()
{
    Out(SYS_DHT | LOG_NOTICE) << "ERR: " << mtid[0] << " " << msg << endl;
}

void ErrMsg::encode(QByteArray &) const
{
}

void ErrMsg::parse(BDictNode *dict)
{
    RPCMsg::parse(dict);
    BListNode *ln = dict->getList(ERR_DHT);
    if (!ln)
        throw bt::Error("Invalid error message");

    msg = ln->getString(1);
}

}
