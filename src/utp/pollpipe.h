/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_POLLPIPE_H
#define UTP_POLLPIPE_H

#include <bitset>
#include <net/poll.h>
#include <net/wakeuppipe.h>

namespace utp
{
/*!
 * \brief Special wake up pipe for UTP polling.
 */
class PollPipe : public net::WakeUpPipe
{
public:
    PollPipe(net::Poll::Mode mode);
    ~PollPipe() override;

    using Ptr = QSharedPointer<PollPipe>;

    //! Is the pipe being polled
    bool polling() const
    {
        return poll_index >= 0;
    }

    //! Prepare the poll
    void prepare(net::Poll *p, bt::Uint16 conn_id, PollPipe::Ptr self)
    {
        QMutexLocker lock(&mutex);
        conn_ids.set(conn_id, true);
        if (poll_index < 0) {
            poll_index = p->add(qSharedPointerCast<PollClient>(self));
        }
    }

    //! Are we polling a connection
    bool polling(bt::Uint16 conn) const
    {
        QMutexLocker lock(&mutex);
        return poll_index >= 0 && conn_ids[conn];
    }

    //! Reset the poll_index
    void reset() override;

    //! Polling mode
    net::Poll::Mode pollingMode() const
    {
        return mode;
    }

private:
    net::Poll::Mode mode;
    int poll_index;
    std::bitset<65536> conn_ids;
};

}

#endif // UTP_POLLPIPE_H
