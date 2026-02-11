/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef NET_POLL_H
#define NET_POLL_H

#include <QSharedPointer>
#include <ktorrent_export.h>
#include <map>
#include <util/constants.h>
#include <vector>

#ifdef Q_OS_WIN
#include <util/win32.h>
#endif

struct pollfd;

namespace net
{
/*!
 * \brief Interface that can be polled.
 */
class KTORRENT_EXPORT PollClient
{
public:
    PollClient()
    {
    }
    virtual ~PollClient()
    {
    }

    //! Get the filedescriptor to poll
    [[nodiscard]] virtual int fd() const = 0;

    //! Handle data
    virtual void handleData() = 0;

    //! Reset the client called after poll finishes
    virtual void reset() = 0;

    using Ptr = QSharedPointer<PollClient>;
};

/*!
 * \brief Handles polling of sockets.
 */
class KTORRENT_EXPORT Poll
{
public:
    Poll();
    virtual ~Poll();

    enum Mode {
        INPUT,
        OUTPUT,
    };

    //! Add a file descriptor to the poll (returns the index of it)
    int add(int fd, Mode mode);

    //! Add a poll client
    int add(PollClient::Ptr pc);

    //! Poll all sockets
    int poll(int timeout = -1);

    //! Check if a socket at an index is read
    [[nodiscard]] bool ready(int index, Mode mode) const;

    //! Reset the poll
    void reset();

private:
    std::vector<struct pollfd> fd_vec;
    bt::Uint32 num_sockets;
    std::map<int, PollClient::Ptr> poll_clients;
};

}

#endif // NET_POLL_H
