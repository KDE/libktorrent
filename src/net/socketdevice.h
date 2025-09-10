/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef NET_SOCKETDEVICE_H
#define NET_SOCKETDEVICE_H

#include <ktorrent_export.h>
#include <net/address.h>
#include <net/poll.h>
#include <util/constants.h>

namespace net
{
/*!
 * \headerfile net/socketdevice.h
 * \brief Interface for classes that implement socket behavior should inherit from.
 */
class SocketDevice
{
public:
    SocketDevice(bt::TransportProtocol proto);
    virtual ~SocketDevice();

    enum State {
        IDLE,
        CONNECTING,
        CONNECTED,
        BOUND,
        CLOSED,
    };

    [[nodiscard]] virtual int fd() const = 0;
    [[nodiscard]] virtual bool ok() const = 0;
    virtual int send(const bt::Uint8 *buf, int len) = 0;
    virtual int recv(bt::Uint8 *buf, int max_len) = 0;
    virtual void close() = 0;
    virtual void setBlocking(bool on) = 0;
    [[nodiscard]] virtual bt::Uint32 bytesAvailable() const = 0;
    virtual bool setTOS(unsigned char type_of_service) = 0;
    virtual bool connectTo(const Address &addr) = 0;
    //! See if a connectTo was succesfull in non blocking mode
    virtual bool connectSuccesFull() = 0;
    [[nodiscard]] virtual const Address &getPeerName() const = 0;
    [[nodiscard]] virtual Address getSockName() const = 0;

    //! Get the used transport protocol for this SocketDevice
    [[nodiscard]] bt::TransportProtocol transportProtocol() const
    {
        return transport_protocol;
    }

    /*!
     * Set the remote address, used by Socks to set the actual address.
     * \param a The address
     */
    void setRemoteAddress(const Address &a)
    {
        addr = a;
        remote_addr_override = true;
    }

    //! reset the socket (i.e. close it and create a new one)
    virtual void reset() = 0;

    [[nodiscard]] State state() const
    {
        return m_state;
    }

    //! Prepare for polling
    virtual void prepare(Poll *p, Poll::Mode mode) = 0;

    //! Check if the socket is ready according to the poll
    virtual bool ready(const Poll *p, Poll::Mode mode) const = 0;

protected:
    State m_state;
    Address addr;
    bool remote_addr_override;
    bt::TransportProtocol transport_protocol;
};

}

#endif // NET_SOCKETDEVICE_H
