/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETSOCKETMONITOR_H
#define NETSOCKETMONITOR_H

#include <ktorrent_export.h>
#include <list>
#include <util/constants.h>

#include <memory>

namespace net
{
using bt::Uint32;

class TrafficShapedSocket;

/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * Monitors all sockets for upload and download traffic.
 * It uses two threads to do this.
 */
class KTORRENT_EXPORT SocketMonitor
{
    SocketMonitor();

public:
    virtual ~SocketMonitor();

    //! Add a new socket, will start the threads if necessary
    void add(TrafficShapedSocket *sock);

    //! Remove a socket, will stop threads if no more sockets are left
    void remove(TrafficShapedSocket *sock);

    typedef std::list<TrafficShapedSocket *>::iterator Itr;

    //! Get the begin of the list of sockets
    Itr begin()
    {
        return sockets.begin();
    }

    //! Get the end of the list of sockets
    Itr end()
    {
        return sockets.end();
    }

    //! lock the monitor
    void lock();

    //! unlock the monitor
    void unlock();

    //! Tell upload thread a packet is ready
    void signalPacketReady();

    enum GroupType {
        UPLOAD_GROUP,
        DOWNLOAD_GROUP,
    };

    /*!
     * Shutdown the socketmonitor and all the networking threads.
     */
    void shutdown();

    /*!
     * Creata a new upload or download group
     * \param type Whether it is an upload or download group
     * \param limit Limit of group in bytes/s
     * \param assured_rate The assured rate in bytes/s
     * \return The group ID
     */
    Uint32 newGroup(GroupType type, Uint32 limit, Uint32 assured_rate);

    /*!
     * Change the group limit
     * \param type The group type
     * \param gid The group id
     * \param limit The limit
     */
    void setGroupLimit(GroupType type, Uint32 gid, Uint32 limit);

    /*!
     * Change the group assured rate
     * \param type The group type
     * \param gid The group id
     * \param as The assured rate
     */
    void setGroupAssuredRate(GroupType type, Uint32 gid, Uint32 as);

    /*!
     * Remove a group
     * \param type The group type
     * \param gid The group id
     */
    void removeGroup(GroupType type, Uint32 gid);

    static void setDownloadCap(Uint32 bytes_per_sec);
    static Uint32 getDownloadCap();
    static void setUploadCap(Uint32 bytes_per_sec);
    static Uint32 getUploadCap();
    static void setSleepTime(Uint32 sleep_time);
    static SocketMonitor &instance()
    {
        return self;
    }

private:
    class Private;
    std::unique_ptr<Private> d;
    std::list<TrafficShapedSocket *> sockets;
    static SocketMonitor self;
};

}

#endif
