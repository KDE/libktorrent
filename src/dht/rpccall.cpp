/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "rpccall.h"
#include "dht.h"
#include "rpcmsg.h"

namespace dht
{
RPCCallListener::RPCCallListener(QObject *parent)
    : QObject(parent)
{
}

RPCCallListener::~RPCCallListener()
{
}

RPCCall::RPCCall(dht::RPCMsg::Ptr msg, bool queued)
    : msg(msg)
    , queued(queued)
{
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &RPCCall::onTimeout);
    if (!queued)
        timer.start(30 * 1000);
}

RPCCall::~RPCCall()
{
}

void RPCCall::start()
{
    queued = false;
    timer.start(30 * 1000);
}

void RPCCall::onTimeout()
{
    timeout(this);
}

void RPCCall::response(RPCMsg::Ptr rsp)
{
    response(this, rsp);
}

Method RPCCall::getMsgMethod() const
{
    if (msg)
        return msg->getMethod();
    else
        return dht::NONE;
}

void RPCCall::addListener(RPCCallListener *cl)
{
    connect(this, qOverload<RPCCall *, RPCMsg::Ptr>(&RPCCall::response), cl, &RPCCallListener::onResponse);
    connect(this, &RPCCall::timeout, cl, &RPCCallListener::onTimeout);
}

}
