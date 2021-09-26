/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "timevalue.h"
#include <sys/time.h>

namespace utp
{
TimeValue::TimeValue()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    seconds = tv.tv_sec;
    microseconds = tv.tv_usec;
}

TimeValue::TimeValue(bt::Uint64 secs, bt::Uint64 usecs)
    : seconds(secs)
    , microseconds(usecs)
{
}

TimeValue::TimeValue(const utp::TimeValue &tv)
    : seconds(tv.seconds)
    , microseconds(tv.microseconds)
{
}

TimeValue &TimeValue::operator=(const utp::TimeValue &tv)
{
    seconds = tv.seconds;
    microseconds = tv.microseconds;
    return *this;
}

}
