/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTUDPTRACKER_H
#define BTUDPTRACKER_H

#include "tracker.h"
#include <QByteArray>
#include <QTimer>
#include <QUrl>
#include <net/address.h>

namespace net
{
class AddressResolver;
}

namespace bt
{
class UDPTrackerSocket;

/*!
 * \headerfile torrent/udptracker.h
 * \author Joris Guisson
 * \brief Communicates with a UDP tracker.
 *
 * This is an implementation of the protocol described in
 * http://xbtt.sourceforge.net/udp_tracker_protocol.html
 */
class UDPTracker : public Tracker
{
    Q_OBJECT
public:
    UDPTracker(const QUrl &url, TrackerDataSource *tds, const PeerID &id, int tier);
    ~UDPTracker() override;

    void start() override;
    void stop(WaitJob *wjob = nullptr) override;
    void completed() override;
    [[nodiscard]] Uint32 failureCount() const override
    {
        return failures;
    }
    void scrape() override;

private Q_SLOTS:
    void onConnTimeout();
    void connectReceived(Int32 tid, Int64 connection_id);
    void announceReceived(Int32 tid, const Uint8 *buf, Uint32 size);
    void scrapeReceived(Int32 tid, const Uint8 *buf, Uint32 size);
    void onError(Int32 tid, const QString &error_string);
    void onResolverResults(net::AddressResolver *ar);
    void manualUpdate() override;

private:
    void sendConnect();
    void sendAnnounce();
    void sendScrape();
    bool doRequest();

    enum Event {
        NONE = 0,
        COMPLETED = 1,
        STARTED = 2,
        STOPPED = 3,
    };

    enum Todo {
        NOTHING = 0,
        SCRAPE_REQUEST = 1,
        ANNOUNCE_REQUEST = 2,
    };

private:
    net::Address address;
    Int64 connection_id;
    Int32 transaction_id;
    Int32 scrape_transaction_id;

    Uint32 data_read;
    int failures;
    bool resolved;
    Uint32 todo;
    Event event;
    QTimer conn_timer;

    static UDPTrackerSocket *socket;
    static Uint32 num_instances;
};

}

#endif
