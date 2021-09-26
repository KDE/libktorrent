/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETSOCKET_H
#define NETSOCKET_H

#include "address.h"
#include <QSharedPointer>
#include <ktorrent_export.h>
#include <net/socketdevice.h>

namespace net
{
const int SEND_FAILURE = 0;
const int SEND_WOULD_BLOCK = -1;

/**
    @author Joris Guisson <joris.guisson@gmail.com>
*/
class KTORRENT_EXPORT Socket : public SocketDevice
{
public:
    explicit Socket(int fd, int ip_version);
    explicit Socket(bool tcp, int ip_version);
    ~Socket() override;

    void setBlocking(bool on) override;
    bool connectTo(const Address &addr) override;
    bool connectSuccesFull() override;
    void close() override;
    Uint32 bytesAvailable() const override;
    int send(const bt::Uint8 *buf, int len) override;
    int recv(bt::Uint8 *buf, int max_len) override;
    bool ok() const override
    {
        return m_fd >= 0;
    }
    int fd() const override
    {
        return m_fd;
    }
    bool setTOS(unsigned char type_of_service) override;
    const Address &getPeerName() const override
    {
        return addr;
    }
    Address getSockName() const override;

    void reset() override;
    void prepare(Poll *p, Poll::Mode mode) override;
    bool ready(const Poll *p, Poll::Mode mode) const override;

    bool bind(const QString &ip, Uint16 port, bool also_listen);
    bool bind(const Address &addr, bool also_listen);
    int accept(Address &a);

    int sendTo(const bt::Uint8 *buf, int size, const Address &addr);
    int recvFrom(bt::Uint8 *buf, int max_size, Address &addr);

    bool isIPv4() const
    {
        return m_ip_version == 4;
    }
    bool isIPv6() const
    {
        return m_ip_version == 6;
    }

    /// Take the filedescriptor from the socket
    int take();

    typedef QSharedPointer<Socket> Ptr;

private:
    void cacheAddress();

private:
    int m_fd;
    int m_ip_version;
    int r_poll_index;
    int w_poll_index;
};

}

#endif
