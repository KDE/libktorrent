/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETSOCKETGROUP_H
#define NETSOCKETGROUP_H

#include <list>
#include <util/constants.h>

namespace net
{
using bt::Uint32;

class TrafficShapedSocket;

/*!
    \author Joris Guisson <joris.guisson@gmail.com>
*/
class SocketGroup
{
    Uint32 limit;
    Uint32 assured_rate;
    std::list<TrafficShapedSocket *> sockets;
    bt::TimeStamp prev_run_time;
    Uint32 group_allowance;
    Uint32 group_assured;

public:
    SocketGroup(Uint32 limit, Uint32 assured_rate);
    virtual ~SocketGroup();

    //! Clear the lists of sockets
    void clear()
    {
        sockets.clear();
    }

    //! Add a socket for processing
    void add(TrafficShapedSocket *s)
    {
        sockets.push_back(s);
    }

    /*!
        Process all the sockets in the vector for download.
        \param global_allowance How much the group can do, this will be updated, 0 means no limit
        \param now Current time
        \return true if we can download more data, false otherwise
    */
    bool download(Uint32 &global_allowance, bt::TimeStamp now);

    /*!
        Process all the sockets in the vector for upload
        \param global_allowance How much the group can do, this will be updated, 0 means no limit
        \param now Current time
        \return true if we can upload more data, false otherwise
     */
    bool upload(Uint32 &global_allowance, bt::TimeStamp now);

    /*!
     * Set the group limit in bytes per sec
     * \param lim The limit
     */
    void setLimit(Uint32 lim)
    {
        limit = lim;
    }

    /*!
     * Set the assured rate for the gorup in bytes per sec
     * \param as The assured rate
     */
    void setAssuredRate(Uint32 as)
    {
        assured_rate = as;
    }

    //! Get the number of sockets
    Uint32 numSockets() const
    {
        return sockets.size();
    }

    /*!
     * Calculate the allowance for this group
     * \param now Current timestamp
     */
    void calcAllowance(bt::TimeStamp now);

    /*!
     * Get the assured allowance .
     */
    Uint32 getAssuredAllowance() const
    {
        return group_assured;
    }

private:
    void processUnlimited(bool up, bt::TimeStamp now);
    bool processLimited(bool up, bt::TimeStamp now, Uint32 &allowance);
    bool process(bool up, bt::TimeStamp now, Uint32 &global_allowance);
};

}

#endif
