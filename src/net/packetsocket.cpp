/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "packetsocket.h"
#include "speed.h"
#include <net/socketmonitor.h>
#include <util/log.h>

using namespace bt;

namespace net
{
PacketSocket::PacketSocket(std::unique_ptr<SocketDevice> sock)
    : TrafficShapedSocket(std::move(sock))
    , ctrl_packets_sent(0)
    , pending_upload_data_bytes(0)
    , uploaded_data_bytes(0)
{
}

PacketSocket::PacketSocket(int fd, int ip_version)
    : TrafficShapedSocket(fd, ip_version)
    , ctrl_packets_sent(0)
    , pending_upload_data_bytes(0)
    , uploaded_data_bytes(0)
{
}

PacketSocket::PacketSocket(bool tcp, int ip_version)
    : TrafficShapedSocket(tcp, ip_version)
    , ctrl_packets_sent(0)
    , pending_upload_data_bytes(0)
    , uploaded_data_bytes(0)
{
}

PacketSocket::~PacketSocket()
{
}

void PacketSocket::selectPacket()
{
    const QMutexLocker locker(&mutex);
    curr_packet.reset();
    // this function should ensure that between
    // each data packet at least 3 control packets are sent
    // so requests can get through

    if (ctrl_packets_sent < 3) {
        // try to send another control packet
        if (control_packets.size() > 0) {
            curr_packet = std::move(control_packets.front());
            control_packets.pop_front();
        } else if (data_packets.size() > 0) {
            curr_packet = std::move(data_packets.front());
            data_packets.pop_front();
        }
    } else {
        if (data_packets.size() > 0) {
            ctrl_packets_sent = 0;
            curr_packet = std::move(data_packets.front());
            data_packets.pop_front();
        } else if (control_packets.size() > 0) {
            curr_packet = std::move(control_packets.front());
            control_packets.pop_front();
        }
    }

    if (curr_packet) {
        preProcess(curr_packet->getData(), curr_packet->getDataLength());
    }
}

Uint32 PacketSocket::write(Uint32 max, bt::TimeStamp now)
{
    if (sock->state() == net::SocketDevice::CONNECTING && !sock->connectSuccesFull()) {
        return 0;
    }

    if (!curr_packet) {
        selectPacket();
    }

    Uint32 written = 0;
    while (curr_packet && (written < max || max == 0)) {
        const Uint32 limit = (max == 0) ? 0 : max - written;
        const int ret = curr_packet->send(sock.get(), limit);
        if (ret > 0) {
            written += ret;
            if (curr_packet->getType() == PIECE) {
                up_speed->onData(ret, now);
                const QMutexLocker locker(&mutex);
                pending_upload_data_bytes -= ret;
                uploaded_data_bytes += ret;
            }
        } else {
            break; // Socket buffer full, so stop sending for now
        }

        if (curr_packet->isSent()) {
            // packet sent, so remove it
            if (curr_packet->getType() == PIECE) {
                // reset ctrl_packets_sent so the next packet should be a ctrl packet
                ctrl_packets_sent = 0;
            } else {
                ctrl_packets_sent++;
            }
            selectPacket();
        } else {
            // we can't write it fully, so break out of loop
            break;
        }
    }

    return written;
}

void PacketSocket::addPacket(Packet packet)
{
    Q_ASSERT(!packet.sending());
    const QMutexLocker locker(&mutex);
    if (packet.getType() == PIECE) {
        pending_upload_data_bytes += packet.getDataLength();
        data_packets.push_back(std::move(packet));
    } else {
        control_packets.push_back(std::move(packet));
    }
    // tell upload thread we have data ready should it be sleeping
    net::SocketMonitor::instance().signalPacketReady();
}

bool PacketSocket::bytesReadyToWrite() const
{
    const QMutexLocker locker(&mutex);
    return !data_packets.empty() || !control_packets.empty() || curr_packet;
}

void PacketSocket::preProcess(Uint8 *data, Uint32 size)
{
    Q_UNUSED(data);
    Q_UNUSED(size);
}

Uint32 PacketSocket::dataBytesUploaded()
{
    const QMutexLocker locker(&mutex);
    const Uint32 ret = uploaded_data_bytes;
    uploaded_data_bytes = 0;
    return ret;
}

void PacketSocket::clearPieces(bool reject)
{
    const QMutexLocker locker(&mutex);

    auto i = data_packets.begin();
    while (i != data_packets.end()) {
        const Packet &p = *i;
        if (p.getType() == bt::PIECE && !p.sending()) {
            if (reject) {
                auto reject_pkt = p.makeRejectOfPiece();
                if (reject_pkt.has_value()) {
                    addPacket(std::move(reject_pkt.value()));
                }
            }
            pending_upload_data_bytes -= p.getDataLength();
            i = data_packets.erase(i);
        } else {
            i++;
        }
    }
}

void PacketSocket::doNotSendPiece(const bt::Request &req, bool reject)
{
    const QMutexLocker locker(&mutex);
    auto i = data_packets.begin();
    while (i != data_packets.end()) {
        const Packet &p = *i;
        if (p.isPiece(req) && !p.sending()) {
            pending_upload_data_bytes -= p.getDataLength();
            if (reject) {
                // queue a reject packet
                auto reject_pkt = p.makeRejectOfPiece();
                if (reject_pkt.has_value()) {
                    addPacket(std::move(reject_pkt.value()));
                }
            }
            i = data_packets.erase(i);
        } else {
            i++;
        }
    }
}

Uint32 PacketSocket::numPendingPieceUploads() const
{
    const QMutexLocker locker(&mutex);
    const bool curr_packet_is_piece = curr_packet && curr_packet->getType() == bt::PIECE;
    return data_packets.size() + (curr_packet_is_piece ? 1 : 0);
}

Uint32 PacketSocket::numPendingPieceUploadBytes() const
{
    const QMutexLocker locker(&mutex);
    return pending_upload_data_bytes;
}

}
