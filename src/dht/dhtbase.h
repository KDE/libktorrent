/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTDHTBASE_H
#define DHTDHTBASE_H

#include <QMap>
#include <QObject>
#include <util/constants.h>

class QString;

namespace bt
{
class SHA1Hash;
}

namespace dht
{
class AnnounceTask;

struct Stats {
    /// number of peers in the routing table
    bt::Uint32 num_peers;
    /// Number of running tasks
    bt::Uint32 num_tasks;
};

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Interface for DHT class, this is to keep other things separate from the inner workings
 * of the DHT.
 */
class DHTBase : public QObject
{
    Q_OBJECT
public:
    DHTBase();
    ~DHTBase() override;

    /**
     * Start the DHT
     * @param table File where the save table is located
     * @param key_file Where our DHT key is stored
     * @param port The port to use
     */
    virtual void start(const QString &table, const QString &key_file, bt::Uint16 port) = 0;

    /**
     * Stop the DHT
     */
    virtual void stop() = 0;

    /**
     * Update the DHT
     */
    virtual void update() = 0;

    /**
     * A Peer has received a PORT message, and uses this function to alert the DHT of it.
     * @param ip The IP of the peer
     * @param port The port in the PORT message
     */
    virtual void portReceived(const QString &ip, bt::Uint16 port) = 0;

    /**
     * Do an announce on the DHT network
     * @param info_hash The info_hash
     * @param port The port
     * @return The task which handles this
     */
    virtual AnnounceTask *announce(const bt::SHA1Hash &info_hash, bt::Uint16 port) = 0;

    /**
     * See if the DHT is running.
     */
    bool isRunning() const
    {
        return running;
    }

    /// Get the DHT port
    bt::Uint16 getPort() const
    {
        return port;
    }

    /// Get statistics about the DHT
    const dht::Stats &getStats() const
    {
        return stats;
    }

    /**
     * Add a DHT node. This node shall be pinged immediately.
     * @param host The hostname or ip
     * @param hport The port of the host
     */
    virtual void addDHTNode(const QString &host, bt::Uint16 hport) = 0;

    /**
     * Returns maxNodes number of <IP address, port> nodes
     * that are closest to ourselves and are good.
     * @param maxNodes maximum nr of nodes in QMap to return.
     */
    virtual QMap<QString, int> getClosestGoodNodes(int maxNodes) = 0;

Q_SIGNALS:
    void started();
    void stopped();

protected:
    bool running;
    bt::Uint16 port;
    dht::Stats stats;
};

}

#endif
