/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPEERSOURCE_H
#define BTPEERSOURCE_H

#include <QList>
#include <QObject>
#include <ktorrent_export.h>
#include <net/address.h>
#include <util/constants.h>

namespace bt
{
class WaitJob;

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * This class is the base class for all classes who which to provide potential peers
 * for torrents. PeerSources should work independently and should emit a signal when they
 * have peers ready.
 */
class KTORRENT_EXPORT PeerSource : public QObject
{
    Q_OBJECT
public:
    PeerSource();
    ~PeerSource() override;

    /**
     * Take the first peer from the list. The item
     * is removed from the list.
     * @param addr The address of the peer
     * @param local Is this is a peer on the local network
     * @return true If there was one available, false if not
     */
    bool takePeer(net::Address &addr, bool &local);

    /**
     * Add a peer to the list of peers.
     * @param addr The address of the peer
     * @param local Whether or not the peer is on the local network
     */
    void addPeer(const net::Address &addr, bool local = false);

public Q_SLOTS:
    /**
     * Start gathering peers.
     */
    virtual void start() = 0;

    /**
     * Stop gathering peers.
     */
    virtual void stop(bt::WaitJob *wjob = nullptr) = 0;

    /**
     * The torrent has finished downloading.
     * This is optional and should be used by HTTP and UDP tracker sources
     * to notify the tracker.
     */
    virtual void completed();

    /**
     * PeerSources wanting to implement a manual update, should implement this.
     */
    virtual void manualUpdate();

    /**
     * The source is about to be destroyed. Subclasses can override this
     * to clean up some things.
     */
    virtual void aboutToBeDestroyed();
Q_SIGNALS:
    /**
     * This signal should be emitted when a new batch of peers is ready.
     * @param ps The PeerSource
     */
    void peersReady(PeerSource *ps);

private:
    /// List to keep the potential peers in.
    QList<QPair<net::Address, bool>> peers;
};

}

#endif
