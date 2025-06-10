/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETSPEED_H
#define NETSPEED_H

#include <QPair>
#include <deque>
#include <util/constants.h>

namespace net
{
/*!
    \author Joris Guisson <joris.guisson@gmail.com>

    \brief Measures the download and upload speed.
*/
class Speed
{
    QAtomicInt rate;
    bt::Uint32 bytes;
    std::deque<QPair<bt::Uint32, bt::TimeStamp>> dlrate;

public:
    Speed();
    virtual ~Speed();

    void onData(bt::Uint32 bytes, bt::TimeStamp ts);
    void update(bt::TimeStamp now);
    int getRate() const
    {
        return rate;
    }
};

}

#endif
