/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETNETWORKTHREAD_H
#define NETNETWORKTHREAD_H

#include <QThread>
#include <net/poll.h>
#include <net/socketgroup.h>
#include <util/constants.h>
#include <util/ptrmap.h>

namespace net
{
class SocketMonitor;

/*!
    \author Joris Guisson <joris.guisson@gmail.com>

    Base class for the 2 networking threads. Handles the socket groups.
*/
class NetworkThread : public QThread, public Poll
{
protected:
    SocketMonitor *sm;
    bool running;
    bt::PtrMap<bt::Uint32, SocketGroup> groups;
    bt::TimeStamp prev_run_time;

public:
    NetworkThread(SocketMonitor *sm);
    ~NetworkThread() override;

    /*!
     * Add a new group with a given limit
     * \param gid The group ID (cannot be 0, 0 is the default group)
     * \param limit The limit in bytes per sec
     * \param assured_rate The assured rate for this group in bytes per second
     */
    void addGroup(bt::Uint32 gid, bt::Uint32 limit, bt::Uint32 assured_rate);

    /*!
     * Remove a group
     * \param gid The group ID
     */
    void removeGroup(bt::Uint32 gid);

    /*!
     * Set the limit for a group
     * \param gid The group ID
     * \param limit The limit
     */
    void setGroupLimit(bt::Uint32 gid, bt::Uint32 limit);

    /*!
     * Set the assured rate for a group
     * \param gid The group ID
     * \param as The assured rate
     */
    void setGroupAssuredRate(bt::Uint32 gid, bt::Uint32 as);

    /*!
     * The main function of the thread
     */
    void run() override;

    /*!
     * Subclasses must implement this function
     */
    virtual void update() = 0;

    /*!
     * Do one SocketGroup
     * \param g The group
     * \param allowance The groups allowance
     * \param now The current time
     * \return true if the group can go again
     */
    virtual bool doGroup(SocketGroup *g, bt::Uint32 &allowance, bt::TimeStamp now) = 0;

    //! Stop before the next update
    void stop()
    {
        running = false;
    }

    //! Is the thread running
    bool isRunning() const
    {
        return running;
    }

protected:
    /*!
     * Go over all groups and do them
     * \param num_ready The number of ready sockets
     * \param now The current time
     * \param limit The global limit in bytes per sec
     */
    void doGroups(bt::Uint32 num_ready, bt::TimeStamp now, bt::Uint32 limit);

private:
    bt::Uint32 doGroupsLimited(bt::Uint32 num_ready, bt::TimeStamp now, bt::Uint32 &allowance);
};

}

#endif
