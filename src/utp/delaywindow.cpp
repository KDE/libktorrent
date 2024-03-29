/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "delaywindow.h"
#include <algorithm>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace utp
{
DelayWindow::DelayWindow()
    : delay_window(100)
{
}

DelayWindow::~DelayWindow()
{
}

bt::Uint32 DelayWindow::update(const utp::Header *hdr, bt::TimeStamp receive_time)
{
    // First cleanup old values at the beginning
    DelayEntryItr itr = delay_window.begin();
    while (itr != delay_window.end()) {
        // drop everything older then 2 minutes
        if (receive_time - itr->receive_time > DELAY_WINDOW_SIZE) {
            // Old entry, can remove it
            itr = delay_window.erase(itr);
        } else
            break;
    }

    // If we are on the end or the new value has a lower delay, clear the list and insert at front
    if (itr == delay_window.end() || hdr->timestamp_difference_microseconds < itr->timestamp_difference_microseconds) {
        delay_window.clear();
        if (delay_window.full())
            delay_window.set_capacity(delay_window.capacity() + 100);

        delay_window.push_back(DelayEntry(hdr->timestamp_difference_microseconds, receive_time));
        return hdr->timestamp_difference_microseconds;
    }

    // Use binary search to find the position where we need to insert
    DelayEntry entry(hdr->timestamp_difference_microseconds, receive_time);
    itr = std::lower_bound(delay_window.begin(), delay_window.end(), entry);
    // Everything until the end has a higher delay then the new sample and is older.
    // So they can all be dropped, because they can never be the minimum delay again.
    if (itr != delay_window.end())
        delay_window.erase(itr, delay_window.end());

    delay_window.push_back(entry);

    // Out(SYS_GEN|LOG_DEBUG) << "Delay window: " << delay_window.size() << endl;
    return delay_window.front().timestamp_difference_microseconds;
}

}
