/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTDHTPEERSOURCE_H
#define DHTDHTPEERSOURCE_H

#include "task.h"
#include <QTimer>
#include <interfaces/peersource.h>

namespace bt
{
class WaitJob;
struct DHTNode;
}

namespace dht
{
class DHTBase;
class AnnounceTask;

/*!
    \author Joris Guisson <joris.guisson@gmail.com>
*/
class KTORRENT_EXPORT DHTPeerSource : public bt::PeerSource
{
public:
    DHTPeerSource(DHTBase &dh_table, const bt::SHA1Hash &info_hash, const QString &torrent_name);
    ~DHTPeerSource() override;

    void start() override;
    void stop(bt::WaitJob *wjob = nullptr) override;
    void manualUpdate() override;

    void addDHTNode(const bt::DHTNode &node);
    void setRequestInterval(bt::Uint32 interval);

private:
    void onTimeout();
    bool doRequest();
    void onDataReady(Task *t);
    void onFinished(Task *t);
    void dhtStopped();

    DHTBase &dh_table;
    AnnounceTask *curr_task;
    bt::SHA1Hash info_hash;
    QTimer timer;
    bool started;
    QList<bt::DHTNode> nodes;
    QString torrent_name;
    bt::Uint32 request_interval;
};

}

#endif
