/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "streamsocket.h"
#include "socketmonitor.h"

namespace net
{
StreamSocket::StreamSocket(bool tcp, int ip_version, StreamSocketListener *listener)
    : TrafficShapedSocket(tcp, ip_version)
    , listener(listener)
{
}

StreamSocket::~StreamSocket()
{
}

void StreamSocket::addData(const QByteArray &data)
{
    QMutexLocker lock(&mutex);
    buffer.append(data);
    net::SocketMonitor::instance().signalPacketReady();
}

bool StreamSocket::bytesReadyToWrite() const
{
    QMutexLocker lock(&mutex);
    return !buffer.isEmpty() || sock->state() == net::SocketDevice::CONNECTING;
}

bt::Uint32 StreamSocket::write(bt::Uint32 max, bt::TimeStamp now)
{
    Q_UNUSED(now);

    QMutexLocker lock(&mutex);
    if (sock->state() == net::SocketDevice::CONNECTING) {
        bool ok = sock->connectSuccesFull();
        if (listener) {
            listener->connectFinished(ok);
        }
        if (!ok) {
            return 0;
        }
    }

    if (buffer.isEmpty()) {
        return 0;
    }

    // max 0 means unlimited transfer, try to send the entire buffer then
    int to_send = (max == 0) ? buffer.size() : qMin<int>(buffer.size(), max);
    int ret = sock->send((const bt::Uint8 *)buffer.data(), to_send);
    if (ret == to_send) {
        buffer.clear();
        if (listener) {
            listener->dataSent();
        }
        return ret;
    } else if (ret > 0) {
        buffer = buffer.mid(ret);
        return ret;
    } else {
        return 0;
    }
}
}
