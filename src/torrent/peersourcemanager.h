/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEERSOURCEMANAGER_H
#define BTPEERSOURCEMANAGER_H

#include <interfaces/torrentinterface.h>
#include <tracker/tracker.h>
#include <tracker/trackermanager.h>
#include <util/constants.h>
#include <util/waitjob.h>

namespace dht
{
class DHTPeerSource;
}

namespace bt
{
class Torrent;
class TorrentControl;
class PeerSource;

/*!
 * \headerfile torrent/peersourcemanager.h
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * \brief Manages all PeerSources for a given torrent.
 */
class PeerSourceManager : public TrackerManager
{
    QList<PeerSource *> additional;
    dht::DHTPeerSource *m_dht;

public:
    PeerSourceManager(TorrentControl *tor, PeerManager *pman);
    ~PeerSourceManager() override;

    /*!
     * Add a PeerSource, the difference between PeerSource and Tracker
     * is that only one Tracker can be used at the same time,
     * PeerSource can always be used.
     * \param ps The PeerSource
     */
    void addPeerSource(PeerSource *ps);

    /*!
     * Remove a Tracker or PeerSource.
     * \param ps
     */
    void removePeerSource(PeerSource *ps);

    /*!
     * See if the PeerSourceManager has been started
     */
    [[nodiscard]] bool isStarted() const
    {
        return started;
    }

    void start() override;
    void stop(WaitJob *wjob = nullptr) override;
    void completed() override;
    void manualUpdate() override;

    //! Adds DHT as PeerSource for this torrent
    void addDHT();
    //! Removes DHT from PeerSourceManager for this torrent.
    void removeDHT();
    //! Checks if DHT is enabled
    bool dhtStarted();
};

}

#endif
