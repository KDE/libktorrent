/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "socketmonitor.h"
#include "downloadthread.h"
#include "trafficshapedsocket.h"
#include "uploadthread.h"

#include <torrent/globals.h>
#include <util/functions.h>
#include <util/log.h>

#include <QRecursiveMutex>
#include <cmath>

using namespace bt;

namespace net
{
SocketMonitor SocketMonitor::self;

class SocketMonitor::Private
{
public:
    Private(SocketMonitor *p)
        : mutex()
        , ut(nullptr)
        , dt(nullptr)
        , next_group_id(1)
    {
        dt = new DownloadThread(p);
        ut = new UploadThread(p);
    }

    ~Private()
    {
        shutdown();
    }

    void shutdown();

    QRecursiveMutex mutex;
    UploadThread *ut;
    DownloadThread *dt;
    Uint32 next_group_id;
};

SocketMonitor::SocketMonitor()
    : d(std::make_unique<Private>(this))
{
}

SocketMonitor::~SocketMonitor()
{
}

void SocketMonitor::shutdown()
{
    d->shutdown();
}

void SocketMonitor::Private::shutdown()
{
    if (ut && ut->isRunning()) {
        ut->stop();
        ut->signalDataReady(); // kick it in the nuts, if the thread is waiting for data
        if (!ut->wait(250)) {
            ut->terminate();
            ut->wait();
        }
    }

    if (dt && dt->isRunning()) {
        dt->stop();
        dt->wakeUp(); // wake it up if necessary
        if (!dt->wait(250)) {
            dt->terminate();
            dt->wait();
        }
    }

    delete ut;
    delete dt;
    ut = nullptr;
    dt = nullptr;
}

void SocketMonitor::lock()
{
    d->mutex.lock();
}

void SocketMonitor::unlock()
{
    d->mutex.unlock();
}

void SocketMonitor::setDownloadCap(Uint32 bytes_per_sec)
{
    DownloadThread::setCap(bytes_per_sec);
}

Uint32 SocketMonitor::getDownloadCap()
{
    return DownloadThread::cap();
}

void SocketMonitor::setUploadCap(Uint32 bytes_per_sec)
{
    UploadThread::setCap(bytes_per_sec);
}

Uint32 SocketMonitor::getUploadCap()
{
    return UploadThread::cap();
}

void SocketMonitor::setSleepTime(Uint32 sleep_time)
{
    DownloadThread::setSleepTime(sleep_time);
    UploadThread::setSleepTime(sleep_time);
}

void SocketMonitor::add(TrafficShapedSocket *sock)
{
    QMutexLocker lock(&d->mutex);

    if (!d->dt || !d->ut) {
        return;
    }

    bool start_threads = sockets.size() == 0;
    sockets.push_back(sock);

    if (start_threads) {
        Out(SYS_CON | LOG_DEBUG) << "Starting socketmonitor threads" << endl;

        if (!d->dt->isRunning()) {
            d->dt->start(QThread::IdlePriority);
        }
        if (!d->ut->isRunning()) {
            d->ut->start(QThread::IdlePriority);
        }
    }
    // wake up download thread so that it can start polling the new socket
    d->dt->wakeUp();
}

void SocketMonitor::remove(TrafficShapedSocket *sock)
{
    QMutexLocker lock(&d->mutex);
    if (sockets.size() == 0) {
        return;
    }

    sockets.remove(sock);
}

void SocketMonitor::signalPacketReady()
{
    if (d->ut) {
        d->ut->signalDataReady();
    }
}

Uint32 SocketMonitor::newGroup(GroupType type, Uint32 limit, Uint32 assured_rate)
{
    QMutexLocker lock(&d->mutex);
    if (!d->dt || !d->ut) {
        return 0;
    }

    Uint32 gid = d->next_group_id++;
    if (type == UPLOAD_GROUP) {
        d->ut->addGroup(gid, limit, assured_rate);
    } else {
        d->dt->addGroup(gid, limit, assured_rate);
    }

    return gid;
}

void SocketMonitor::setGroupLimit(GroupType type, Uint32 gid, Uint32 limit)
{
    QMutexLocker lock(&d->mutex);
    if (!d->dt || !d->ut) {
        return;
    }

    if (type == UPLOAD_GROUP) {
        d->ut->setGroupLimit(gid, limit);
    } else {
        d->dt->setGroupLimit(gid, limit);
    }
}

void SocketMonitor::setGroupAssuredRate(GroupType type, Uint32 gid, Uint32 as)
{
    QMutexLocker lock(&d->mutex);
    if (!d->dt || !d->ut) {
        return;
    }

    if (type == UPLOAD_GROUP) {
        d->ut->setGroupAssuredRate(gid, as);
    } else {
        d->dt->setGroupAssuredRate(gid, as);
    }
}

void SocketMonitor::removeGroup(GroupType type, Uint32 gid)
{
    QMutexLocker lock(&d->mutex);
    if (!d->dt || !d->ut) {
        return;
    }

    if (type == UPLOAD_GROUP) {
        d->ut->removeGroup(gid);
    } else {
        d->dt->removeGroup(gid);
    }
}

}
