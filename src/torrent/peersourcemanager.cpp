/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "peersourcemanager.h"

#include <KLocalizedString>
#include <QFile>
#include <QTextStream>

// #include <functions.h>
#include "torrent.h"
#include "torrentcontrol.h"
#include <dht/dhtbase.h>
#include <dht/dhtpeersource.h>
#include <peer/peermanager.h>
#include <torrent/globals.h>
#include <tracker/tracker.h>
#include <util/log.h>

namespace bt
{
PeerSourceManager::PeerSourceManager(TorrentControl *tor, PeerManager *pman)
    : TrackerManager(tor, pman)
    , m_dht(nullptr)
{
}

PeerSourceManager::~PeerSourceManager()
{
    QList<PeerSource *>::iterator itr = additional.begin();
    while (itr != additional.end()) {
        PeerSource *ps = *itr;
        ps->aboutToBeDestroyed();
        ++itr;
    }
    qDeleteAll(additional);
    additional.clear();
}

void PeerSourceManager::addPeerSource(PeerSource *ps)
{
    additional.append(ps);
    connect(ps, &PeerSource::peersReady, pman, &PeerManager::peerSourceReady);
}

void PeerSourceManager::removePeerSource(PeerSource *ps)
{
    disconnect(ps, &PeerSource::peersReady, pman, &PeerManager::peerSourceReady);
    additional.removeAll(ps);
}

void PeerSourceManager::start()
{
    if (started) {
        return;
    }

    QList<PeerSource *>::iterator i = additional.begin();
    while (i != additional.end()) {
        (*i)->start();
        ++i;
    }

    TrackerManager::start();
}

void PeerSourceManager::stop(WaitJob *wjob)
{
    if (!started) {
        return;
    }

    QList<PeerSource *>::iterator i = additional.begin();
    while (i != additional.end()) {
        (*i)->stop();
        ++i;
    }

    TrackerManager::stop(wjob);
}

void PeerSourceManager::completed()
{
    QList<PeerSource *>::iterator i = additional.begin();
    while (i != additional.end()) {
        (*i)->completed();
        ++i;
    }

    TrackerManager::completed();
}

void PeerSourceManager::manualUpdate()
{
    QList<PeerSource *>::iterator i = additional.begin();
    while (i != additional.end()) {
        (*i)->manualUpdate();
        ++i;
    }

    TrackerManager::manualUpdate();
}

void PeerSourceManager::addDHT()
{
    if (m_dht) {
        removePeerSource(m_dht);
        delete m_dht;
    }

    m_dht = new dht::DHTPeerSource(Globals::instance().getDHT(), tor->getInfoHash(), tor->getStats().torrent_name);
    for (Uint32 i = 0; i < tor->getNumDHTNodes(); i++) {
        m_dht->addDHTNode(tor->getDHTNode(i));
    }

    // add the DHT source
    addPeerSource(m_dht);
}

void PeerSourceManager::removeDHT()
{
    if (!m_dht) {
        return;
    }

    removePeerSource(m_dht);
    delete m_dht;
    m_dht = nullptr;
}

bool PeerSourceManager::dhtStarted()
{
    return m_dht != nullptr;
}

}
