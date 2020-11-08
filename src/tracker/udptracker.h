/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#ifndef BTUDPTRACKER_H
#define BTUDPTRACKER_H

#include <QUrl>
#include <QTimer>
#include <QByteArray>
#include <net/address.h>
#include "tracker.h"

namespace net
{
class AddressResolver;
}

namespace bt
{

class UDPTrackerSocket;

/**
 * @author Joris Guisson
 * @brief Communicates with an UDP tracker
 *
 * This class is able to communicate with an UDP tracker.
 * This is an implementation of the protocol described in
 * http://xbtt.sourceforge.net/udp_tracker_protocol.html
 */
class UDPTracker : public Tracker
{
    Q_OBJECT
public:
    UDPTracker(const QUrl &url, TrackerDataSource* tds, const PeerID & id, int tier);
    ~UDPTracker() override;

    void start() override;
    void stop(WaitJob* wjob = 0) override;
    void completed() override;
    Uint32 failureCount() const override
    {
        return failures;
    }
    void scrape() override;

private Q_SLOTS:
    void onConnTimeout();
    void connectReceived(Int32 tid, Int64 connection_id);
    void announceReceived(Int32 tid, const Uint8* buf, Uint32 size);
    void scrapeReceived(Int32 tid, const Uint8* buf, Uint32 size);
    void onError(Int32 tid, const QString & error_string);
    void onResolverResults(net::AddressResolver* ar);
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
        STOPPED = 3
    };

    enum Todo {
        NOTHING = 0,
        SCRAPE_REQUEST = 1,
        ANNOUNCE_REQUEST = 2
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

    static UDPTrackerSocket* socket;
    static Uint32 num_instances;
};

}

#endif
