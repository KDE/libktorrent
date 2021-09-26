/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_UTPSOCKET_H
#define UTP_UTPSOCKET_H

#include <ktorrent_export.h>
#include <net/socketdevice.h>
#include <util/constants.h>
#include <utp/connection.h>

namespace utp
{
/**
    UTPSocket class serves as an interface for the networking code.
*/
class KTORRENT_EXPORT UTPSocket : public net::SocketDevice
{
public:
    UTPSocket();
    UTPSocket(Connection::WPtr conn);
    ~UTPSocket() override;

    int fd() const override;
    bool ok() const override;
    int send(const bt::Uint8 *buf, int len) override;
    int recv(bt::Uint8 *buf, int max_len) override;
    void close() override;
    void setBlocking(bool on) override;
    bt::Uint32 bytesAvailable() const override;
    bool setTOS(unsigned char type_of_service) override;
    bool connectTo(const net::Address &addr) override;
    bool connectSuccesFull() override;
    const net::Address &getPeerName() const override;
    net::Address getSockName() const override;
    void reset() override;
    void prepare(net::Poll *p, net::Poll::Mode mode) override;
    bool ready(const net::Poll *p, net::Poll::Mode mode) const override;

private:
    Connection::WPtr conn;
    bool blocking;
    mutable bool polled_for_reading;
    mutable bool polled_for_writing;
};
}

#endif // UTPSOCKET_H
