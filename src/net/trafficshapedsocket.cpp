/*
    SPDX-FileCopyrightText: 2011 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "trafficshapedsocket.h"

#include "socket.h"
#include "speed.h"
#include <QStringList>
#include <util/functions.h>

const bt::Uint32 OUTPUT_BUFFER_SIZE = 16393;

using namespace bt;

namespace net
{
TrafficShapedSocket::TrafficShapedSocket(SocketDevice *sock)
    : rdr(nullptr)
    , up_gid(0)
    , down_gid(0)
    , sock(sock)
    , mutex()
{
    down_speed = new Speed();
    up_speed = new Speed();
}

TrafficShapedSocket::TrafficShapedSocket(int fd, int ip_version)
    : rdr(nullptr)
    , up_gid(0)
    , down_gid(0)
    , mutex()
{
    sock = new Socket(fd, ip_version);
    down_speed = new Speed();
    up_speed = new Speed();
}

TrafficShapedSocket::TrafficShapedSocket(bool tcp, int ip_version)
    : rdr(nullptr)
    , up_gid(0)
    , down_gid(0)
    , mutex()
{
    Socket *socket = new Socket(tcp, ip_version);

    QString iface = NetworkInterface();
    QStringList ips = NetworkInterfaceIPAddresses(iface);
    if (ips.size() > 0)
        socket->bind(ips.front(), 0, false);

    sock = socket;
    down_speed = new Speed();
    up_speed = new Speed();
}

TrafficShapedSocket::~TrafficShapedSocket()
{
    delete up_speed;
    delete down_speed;
    delete sock;
}

void TrafficShapedSocket::setGroupID(Uint32 gid, bool upload)
{
    if (upload)
        up_gid = gid;
    else
        down_gid = gid;
}

int TrafficShapedSocket::getDownloadRate() const
{
    // getRate is atomic
    return down_speed->getRate();
}

int TrafficShapedSocket::getUploadRate() const
{
    // getRate is atomic
    return up_speed->getRate();
}

void TrafficShapedSocket::updateSpeeds(bt::TimeStamp now)
{
    mutex.lock();
    up_speed->update(now);
    down_speed->update(now);
    mutex.unlock();
}

static bt::Uint8 input_buffer[OUTPUT_BUFFER_SIZE];

Uint32 TrafficShapedSocket::read(bt::Uint32 max_bytes_to_read, bt::TimeStamp now)
{
    Uint32 br = 0;
    bool no_limit = (max_bytes_to_read == 0);
    Uint32 ba = sock->bytesAvailable();
    if (ba == 0) {
        // For some strange reason, sometimes bytesAvailable returns 0, while there are
        // bytes to read, so give ba the maximum value it can be
        ba = max_bytes_to_read > 0 ? max_bytes_to_read : OUTPUT_BUFFER_SIZE;
    }

    while ((br < max_bytes_to_read || no_limit) && ba > 0) {
        Uint32 tr = ba;
        if (tr > OUTPUT_BUFFER_SIZE)
            tr = OUTPUT_BUFFER_SIZE;
        if (!no_limit && tr + br > max_bytes_to_read)
            tr = max_bytes_to_read - br;

        int ret = sock->recv(input_buffer, tr);
        if (ret > 0) {
            mutex.lock();
            down_speed->onData(ret, now);
            mutex.unlock();
            if (rdr) {
                postProcess(input_buffer, ret);
                rdr->onDataReady(input_buffer, ret);
            }
            br += ret;
            ba -= ret;
        } else if (ret < 0) {
            return br;
        } else {
            sock->close();
            return br;
        }
    }
    return br;
}

void TrafficShapedSocket::postProcess(Uint8 *data, Uint32 size)
{
    Q_UNUSED(data);
    Q_UNUSED(size);
}
}
