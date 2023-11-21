/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "reverseresolver.h"
#include <netdb.h>
#include <sys/socket.h>

namespace net
{
ReverseResolverThread *ReverseResolver::worker = nullptr;

ReverseResolver::ReverseResolver(QObject *parent)
    : QObject(parent)
{
}

ReverseResolver::~ReverseResolver()
{
}

void ReverseResolver::resolveAsync(const net::Address &addr)
{
    addr_to_resolve = addr;
    if (!worker) {
        worker = new ReverseResolverThread();
        worker->add(this);
        worker->start();
    } else {
        worker->add(this);
    }
}

QString ReverseResolver::resolve(const net::Address &addr)
{
    struct sockaddr_storage ss;
    int slen = 0;
    addr.toSocketAddress(&ss, slen);

    char host[200];
    char service[200];
    memset(host, 0, 200);
    memset(service, 0, 200);
    if (getnameinfo((struct sockaddr *)&ss, slen, host, 199, service, 199, NI_NAMEREQD) == 0)
        return QString::fromUtf8(host);
    else
        return QString();
}

void ReverseResolver::run()
{
    QString res = resolve(addr_to_resolve);
    Q_EMIT resolved(res);
}

void ReverseResolver::shutdown()
{
    if (worker) {
        worker->stop();
        worker->wait();
        delete worker;
        worker = nullptr;
    }
}

ReverseResolverThread::ReverseResolverThread()
    : stopped(false)
{
}

ReverseResolverThread::~ReverseResolverThread()
{
}

void ReverseResolverThread::add(ReverseResolver *rr)
{
    mutex.lock();
    todo_list.append(rr);
    mutex.unlock();
    more_data.wakeOne();
}

void ReverseResolverThread::run()
{
    while (!stopped) {
        mutex.lock();
        if (!todo_list.empty()) {
            ReverseResolver *rr = todo_list.first();
            todo_list.pop_front();
            mutex.unlock();
            rr->run();
            rr->deleteLater();
        } else {
            more_data.wait(&mutex);
            mutex.unlock();
        }
    }

    // cleanup if necessary
    for (ReverseResolver *rr : std::as_const(todo_list))
        rr->deleteLater();
    todo_list.clear();
}

void ReverseResolverThread::stop()
{
    stopped = true;
    // wake up the thread if it is sleeping
    more_data.wakeOne();
}

}

#include "moc_reverseresolver.cpp"
