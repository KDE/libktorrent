/***************************************************************************
 *   Copyright (C) 2010 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "delaywindow.h"
#include <algorithm>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace utp
{

	DelayWindow::DelayWindow()
	{
	}

	DelayWindow::~DelayWindow()
	{

	}

	bt::Uint32 DelayWindow::update(const utp::Header* hdr, bt::TimeStamp receive_time)
	{
		// First cleanup old values at the beginning
		DelayEntryItr itr = delay_window.begin();
		while (itr != delay_window.end())
		{
			// drop everything older then 2 minutes
			if (receive_time - itr->receive_time > DELAY_WINDOW_SIZE)
			{
				// Old entry, can remove it
				itr = delay_window.erase(itr);
			}
			else
				break;
		}

		// If we are on the end or the new value has a lower delay, clear the list and insert at front
		if (itr == delay_window.end() || hdr->timestamp_difference_microseconds < itr->timestamp_difference_microseconds)
		{
			delay_window.clear();
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

		//Out(SYS_GEN|LOG_DEBUG) << "Delay window: " << delay_window.size() << endl;
		return delay_window.front().timestamp_difference_microseconds;
	}

}

