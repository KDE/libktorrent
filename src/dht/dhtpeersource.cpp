/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "dhtpeersource.h"
#include "announcetask.h"
#include "dht.h"
#include <QHostAddress>
#include <interfaces/torrentinterface.h>
#include <torrent/globals.h>
#include <torrent/server.h>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace dht
{
DHTPeerSource::DHTPeerSource(DHTBase &dh_table, const bt::SHA1Hash &info_hash, const QString &torrent_name)
    : dh_table(dh_table)
    , curr_task(nullptr)
    , info_hash(info_hash)
    , torrent_name(torrent_name)
{
    connect(&timer, &QTimer::timeout, this, &DHTPeerSource::onTimeout);
    connect(&dh_table, &DHTBase::started, this, &DHTPeerSource::manualUpdate);
    connect(&dh_table, &DHTBase::stopped, this, &DHTPeerSource::dhtStopped);
    started = false;
    timer.setSingleShot(true);
    request_interval = 5 * 60 * 1000;
}

DHTPeerSource::~DHTPeerSource()
{
    if (curr_task)
        curr_task->kill();
}

void DHTPeerSource::start()
{
    started = true;
    if (dh_table.isRunning())
        doRequest();
}

void DHTPeerSource::dhtStopped()
{
    stop(nullptr);
    curr_task = nullptr;
}

void DHTPeerSource::stop(bt::WaitJob *)
{
    started = false;
    if (curr_task) {
        curr_task->kill();
        timer.stop();
    }
}

void DHTPeerSource::manualUpdate()
{
    if (dh_table.isRunning() && started)
        doRequest();
}

bool DHTPeerSource::doRequest()
{
    if (!dh_table.isRunning())
        return false;

    if (curr_task)
        return true;

    Uint16 port = ServerInterface::getPort();
    curr_task = dh_table.announce(info_hash, port);
    if (curr_task) {
        for (const bt::DHTNode &n : std::as_const(nodes))
            curr_task->addDHTNode(n.ip, n.port);

        connect(curr_task, &AnnounceTask::dataReady, this, &DHTPeerSource::onDataReady);
        connect(curr_task, &AnnounceTask::finished, this, &DHTPeerSource::onFinished);
        return true;
    }

    return false;
}

void DHTPeerSource::onFinished(Task *t)
{
    if (curr_task == t) {
        onDataReady(curr_task);
        curr_task = nullptr;
        timer.start(request_interval);
    }
}

void DHTPeerSource::onDataReady(Task *t)
{
    if (curr_task == t) {
        Uint32 cnt = 0;
        DBItem item;
        while (curr_task->takeItem(item)) {
            const net::Address &addr = item.getAddress();
            /*  Out(SYS_DHT|LOG_NOTICE) <<
                        QString("DHT: Got potential peer %1 for torrent %2")
                        .arg(addr.toString()).arg(tor->getStats().torrent_name) << endl;*/
            addPeer(addr, false);
            cnt++;
        }

        if (cnt) {
            Out(SYS_DHT | LOG_NOTICE) << QString("DHT: Got %1 potential peers for torrent %2").arg(cnt).arg(torrent_name) << endl;
            peersReady(this);
        }
    }
}

void DHTPeerSource::onTimeout()
{
    if (dh_table.isRunning() && started)
        doRequest();
}

void DHTPeerSource::addDHTNode(const bt::DHTNode &node)
{
    nodes.append(node);
}

void DHTPeerSource::setRequestInterval(Uint32 interval)
{
    request_interval = interval;
}

}
