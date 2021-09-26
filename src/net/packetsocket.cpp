/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#include "packetsocket.h"
#include "speed.h"
#include <net/socketmonitor.h>
#include <util/log.h>

using namespace bt;

namespace net
{
PacketSocket::PacketSocket(SocketDevice *sock)
    : TrafficShapedSocket(sock)
    , ctrl_packets_sent(0)
    , uploaded_data_bytes(0)
{
}

PacketSocket::PacketSocket(int fd, int ip_version)
    : TrafficShapedSocket(fd, ip_version)
    , ctrl_packets_sent(0)
    , uploaded_data_bytes(0)
{
}

PacketSocket::PacketSocket(bool tcp, int ip_version)
    : TrafficShapedSocket(tcp, ip_version)
    , ctrl_packets_sent(0)
    , uploaded_data_bytes(0)
{
}

PacketSocket::~PacketSocket()
{
}

Packet::Ptr PacketSocket::selectPacket()
{
    QMutexLocker locker(&mutex);
    Packet::Ptr ret(nullptr);
    // this function should ensure that between
    // each data packet at least 3 control packets are sent
    // so requests can get through

    if (ctrl_packets_sent < 3) {
        // try to send another control packet
        if (control_packets.size() > 0)
            ret = control_packets.front();
        else if (data_packets.size() > 0)
            ret = data_packets.front();
    } else {
        if (data_packets.size() > 0) {
            ctrl_packets_sent = 0;
            ret = data_packets.front();
        } else if (control_packets.size() > 0)
            ret = control_packets.front();
    }

    if (ret)
        preProcess(ret);

    return ret;
}

Uint32 PacketSocket::write(Uint32 max, bt::TimeStamp now)
{
    if (sock->state() == net::SocketDevice::CONNECTING && !sock->connectSuccesFull())
        return 0;

    if (!curr_packet)
        curr_packet = selectPacket();

    Uint32 written = 0;
    while (curr_packet && (written < max || max == 0)) {
        Uint32 limit = (max == 0) ? 0 : max - written;
        int ret = curr_packet->send(sock, limit);
        if (ret > 0) {
            written += ret;
            QMutexLocker locker(&mutex);
            if (curr_packet->getType() == PIECE) {
                up_speed->onData(ret, now);
                uploaded_data_bytes += ret;
            }
        } else
            break; // Socket buffer full, so stop sending for now

        if (curr_packet->isSent()) {
            // packet sent, so remove it
            if (curr_packet->getType() == PIECE) {
                QMutexLocker locker(&mutex);
                if (!data_packets.empty())
                    data_packets.pop_front();
                // reset ctrl_packets_sent so the next packet should be a ctrl packet
                ctrl_packets_sent = 0;
            } else {
                QMutexLocker locker(&mutex);
                if (!control_packets.empty())
                    control_packets.pop_front();
                ctrl_packets_sent++;
            }
            curr_packet = selectPacket();
        } else {
            // we can't write it fully, so break out of loop
            break;
        }
    }

    return written;
}

void PacketSocket::addPacket(Packet::Ptr packet)
{
    QMutexLocker locker(&mutex);
    if (packet->getType() == PIECE)
        data_packets.push_back(packet);
    else
        control_packets.push_back(packet);
    // tell upload thread we have data ready should it be sleeping
    net::SocketMonitor::instance().signalPacketReady();
}

bool PacketSocket::bytesReadyToWrite() const
{
    QMutexLocker locker(&mutex);
    return !data_packets.empty() || !control_packets.empty();
}

void PacketSocket::preProcess(Packet::Ptr packet)
{
    Q_UNUSED(packet);
}

Uint32 PacketSocket::dataBytesUploaded()
{
    QMutexLocker locker(&mutex);
    Uint32 ret = uploaded_data_bytes;
    uploaded_data_bytes = 0;
    return ret;
}

void PacketSocket::clearPieces(bool reject)
{
    QMutexLocker locker(&mutex);

    auto i = data_packets.begin();
    while (i != data_packets.end()) {
        Packet::Ptr p = *i;
        if (p->getType() == bt::PIECE && !p->sending() && curr_packet != p) {
            if (reject)
                addPacket(Packet::Ptr(p->makeRejectOfPiece()));

            i = data_packets.erase(i);
        } else {
            i++;
        }
    }
}

void PacketSocket::doNotSendPiece(const bt::Request &req, bool reject)
{
    QMutexLocker locker(&mutex);
    auto i = data_packets.begin();
    while (i != data_packets.end()) {
        Packet::Ptr p = *i;
        if (p->isPiece(req) && !p->sending() && p != curr_packet) {
            i = data_packets.erase(i);
            if (reject) {
                // queue a reject packet
                addPacket(Packet::Ptr(p->makeRejectOfPiece()));
            }
        } else {
            i++;
        }
    }
}

Uint32 PacketSocket::numPendingPieceUploads() const
{
    QMutexLocker locker(&mutex);
    return data_packets.size();
}

}
