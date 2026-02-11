/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_TIMEVALUE_H
#define UTP_TIMEVALUE_H

#include <ktorrent_export.h>
#include <util/constants.h>

namespace utp
{
/*!
 * \brief High precision time value.
 */
class KTORRENT_EXPORT TimeValue
{
public:
    //! Default constructor, gets the current time
    TimeValue();
    TimeValue(bt::Uint64 secs, bt::Uint64 usecs);
    TimeValue(const TimeValue &tv);

    TimeValue &operator=(const TimeValue &tv);

    [[nodiscard]] bt::Uint32 timestampMicroSeconds() const
    {
        bt::Uint64 microsecs = seconds * 1000000 + microseconds;
        // return microsecs & 0x00000000FFFFFFFF;
        return microsecs;
    }

    void addMilliSeconds(bt::Uint32 ms)
    {
        microseconds += ms * 1000;
        if (microseconds > 1000000) {
            seconds += microseconds / 1000000;
            microseconds = microseconds % 1000000;
        }
    }

    //! Convert to time stamp
    [[nodiscard]] bt::TimeStamp toTimeStamp() const
    {
        return seconds * 1000 + (bt::Uint64)microseconds * 0.001;
    }

public:
    bt::Uint64 seconds;
    bt::Uint64 microseconds;
};

//! Calculate the a - b in milliseconds
inline bt::Int64 operator-(const TimeValue &a, const TimeValue &b)
{
    bt::Int64 seconds = a.seconds - b.seconds;
    bt::Int64 microseconds = a.microseconds - b.microseconds;

    while (microseconds < 0) {
        microseconds += 1000000;
        seconds -= 1;
    }

    return (1000000LL * seconds + microseconds) / 1000;
}

inline bool operator<(const TimeValue &a, const TimeValue &b)
{
    if (a.seconds < b.seconds)
        return true;
    else if (a.seconds == b.seconds)
        return a.microseconds < b.microseconds;
    else
        return false;
}

inline bool operator<=(const TimeValue &a, const TimeValue &b)
{
    if (a.seconds < b.seconds)
        return true;
    else if (a.seconds == b.seconds)
        return a.microseconds <= b.microseconds;
    else
        return false;
}

inline bool operator>(const TimeValue &a, const TimeValue &b)
{
    if (a.seconds > b.seconds)
        return true;
    else if (a.seconds == b.seconds)
        return a.microseconds > b.microseconds;
    else
        return false;
}

inline bool operator>=(const TimeValue &a, const TimeValue &b)
{
    if (a.seconds > b.seconds)
        return true;
    else if (a.seconds == b.seconds)
        return a.microseconds >= b.microseconds;
    else
        return false;
}

}

#endif // UTP_TIMEVALUE_H
