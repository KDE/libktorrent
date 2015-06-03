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

#ifndef UTP_POLLPIPE_H
#define UTP_POLLPIPE_H

#include <bitset>
#include <net/poll.h>
#include <net/wakeuppipe.h>


namespace utp
{

	/**
		Special wake up pipe for UTP polling
	*/
	class PollPipe : public net::WakeUpPipe
	{
	public:
		PollPipe(net::Poll::Mode mode);
		virtual ~PollPipe();

		typedef QSharedPointer<PollPipe> Ptr;

		/// Is the pipe being polled
		bool polling() const {return poll_index >= 0;}

		/// Prepare the poll
		void prepare(net::Poll* p, bt::Uint16 conn_id, PollPipe::Ptr self)
		{
			QMutexLocker lock(&mutex);
			conn_ids.set(conn_id, true);
			if (poll_index < 0)
				poll_index = p->add(qSharedPointerCast<PollClient>(self));
		}

		/// Are we polling a connection
		bool polling(bt::Uint16 conn) const
		{
			QMutexLocker lock(&mutex);
			return poll_index >= 0 && conn_ids[conn];
		}

		/// Reset the poll_index
		virtual void reset();

		/// Polling mode
		net::Poll::Mode pollingMode() const {return mode;}

	private:
		net::Poll::Mode mode;
		int poll_index;
		std::bitset<65536> conn_ids;
	};


}

#endif // UTP_POLLPIPE_H
