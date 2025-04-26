/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_OUTPUTQUEUE_H
#define UTP_OUTPUTQUEUE_H

#include "connection.h"
#include "packetbuffer.h"

#include <QList>
#include <QRecursiveMutex>

#include <deque>
#include <net/serversocket.h>

namespace utp
{
/**
 * Manages the send queue of all UTP server sockets
 */
class OutputQueue
{
public:
    OutputQueue();
    virtual ~OutputQueue();

    /**
     * Add an entry to the queue.
     * @param packet The packet
     * @param conn The connection this packet belongs to
     * @return The number of queued packets
     */
    int add(const PacketBuffer &packet, Connection::WPtr conn);

    /**
     * Attempt to send the queue on a socket
     * @param sock The socket
     */
    void send(net::ServerSocket *sock);

private:
    struct Entry {
        PacketBuffer data;
        Connection::WPtr conn;

        Entry(const PacketBuffer &data, Connection::WPtr conn)
            : data(data)
            , conn(conn)
        {
        }
    };

#ifndef DO_NOT_USE_DEQUE
    std::deque<Entry> queue;
#else
    QList<Entry> queue;
#endif
    QRecursiveMutex mutex;
};

}

#endif // UTP_OUTPUTQUEUE_H
