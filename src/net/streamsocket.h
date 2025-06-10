/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef NET_STREAMSOCKET_H
#define NET_STREAMSOCKET_H

#include <QByteArray>
#include <net/trafficshapedsocket.h>

namespace net
{
/*!
 * \brief Interface for receiving notifications when a StreamSocket sends data.
 */
class StreamSocketListener
{
public:
    virtual ~StreamSocketListener()
    {
    }

    /*!
     * Called when a StreamSocket gets connected.
     */
    virtual void connectFinished(bool succeeded) = 0;

    /*!
     * Called when all data has been sent.
     */
    virtual void dataSent() = 0;
};

/*!
 * \brief TrafficShapedSocket which provides a simple buffer as outbound data queue,
 * and a callback interface (StreamSocketListener) for notification of events.
 */
class StreamSocket : public net::TrafficShapedSocket
{
public:
    StreamSocket(bool tcp, int ip_version, StreamSocketListener *listener);
    ~StreamSocket() override;

    bool bytesReadyToWrite() const override;
    bt::Uint32 write(bt::Uint32 max, bt::TimeStamp now) override;

    /*!
     * Add data to send
     * \param data The QByteArray
     */
    void addData(const QByteArray &data);

private:
    StreamSocketListener *listener;
    QByteArray buffer;
};

}

#endif // NET_STREAMSOCKET_H
