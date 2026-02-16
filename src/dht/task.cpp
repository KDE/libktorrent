/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "task.h"
#include "kclosestnodessearch.h"
#include "rpcserver.h"
#include <net/addressresolver.h>

namespace dht
{
Task::Task(RPCServer *rpc, Node *node, QObject *parent)
    : RPCCallListener(parent)
    , node(node)
    , rpc(rpc)
    , outstanding_reqs(0)
    , task_finished(false)
    , queued(true)
{
}

Task::~Task()
{
}

void Task::start(const KClosestNodesSearch &kns, bool queued)
{
    // fill the todo list
    for (KClosestNodesSearch::CItr i = kns.begin(); i != kns.end(); ++i) {
        todo.insert(i->second);
    }
    this->queued = queued;
    if (!queued) {
        update();
    }
}

void Task::start()
{
    if (queued) {
        queued = false;
        update();
    }
}

void Task::onResponse(dht::RPCCall *c, dht::RPCMsg *rsp)
{
    if (outstanding_reqs > 0) {
        outstanding_reqs--;
    }

    if (!isFinished()) {
        callFinished(c, rsp);

        if (canDoRequest() && !isFinished()) {
            update();
        }
    }
}

void Task::onTimeout(RPCCall *c)
{
    if (outstanding_reqs > 0) {
        outstanding_reqs--;
    }

    if (!isFinished()) {
        callTimeout(c);

        if (canDoRequest() && !isFinished()) {
            update();
        }
    }
}

bool Task::rpcCall(std::unique_ptr<dht::RPCMsg> req)
{
    if (!canDoRequest()) {
        return false;
    }

    RPCCall *c = rpc->doCall(std::move(req));
    c->addListener(this);
    outstanding_reqs++;
    return true;
}

void Task::done()
{
    task_finished = true;
    Q_EMIT finished(this);
}

void Task::emitDataReady()
{
    Q_EMIT dataReady(this);
}

void Task::kill()
{
    task_finished = true;
    Q_EMIT finished(this);
}

void Task::addDHTNode(const QString &ip, bt::Uint16 port)
{
    net::Address addr;
    if (addr.setAddress(ip)) {
        addr.setPort(port);
        todo.insert(KBucketEntry(addr, dht::Key()));
    } else {
        net::AddressResolver::resolve(ip, port, this, SLOT(onResolverResults(net::AddressResolver *)));
    }
}

void Task::onResolverResults(net::AddressResolver *ar)
{
    if (!ar->succeeded()) {
        return;
    }

    todo.insert(KBucketEntry(ar->address(), dht::Key()));
}

}

#include "moc_task.cpp"
