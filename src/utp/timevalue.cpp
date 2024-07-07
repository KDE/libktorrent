/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "timevalue.h"
#include <chrono>

namespace utp
{
TimeValue::TimeValue()
{
    const auto since_epoch = std::chrono::system_clock::now().time_since_epoch();
    const auto seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    const auto microseconds_remainder = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch - seconds_since_epoch);
    seconds = seconds_since_epoch.count();
    microseconds = microseconds_remainder.count();
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
