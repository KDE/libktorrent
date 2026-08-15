/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "packetreader.h"
#include "peer.h"
#include <QtAlgorithms>
#include <util/endian.h>
#include <util/file.h>
#include <util/log.h>
#include <utility>

namespace bt
{
IncomingPacket::IncomingPacket(Uint32 size) noexcept
    : data(size)
{
}

PacketReader::PacketReader(Uint32 max_packet_size)
    : error(false)
    , len_received(-1)
    , max_packet_size(max_packet_size)
{
}

PacketReader::~PacketReader()
{
}

std::optional<IncomingPacket> PacketReader::dequeuePacket()
{
    const QMutexLocker lock(&mutex);
    if (packet_queue.empty()) {
        return std::nullopt;
    }

    IncomingPacket &pck = packet_queue.front();
    if (pck.read != pck.data.size()) {
        return std::nullopt;
    }

    IncomingPacket p(std::move(pck));
    packet_queue.pop_front();
    return p;
}

void PacketReader::update(PeerInterface &peer)
{
    if (error) {
        return;
    }

    std::optional<IncomingPacket> pck = dequeuePacket();
    while (pck.has_value()) {
        peer.handlePacket(pck->data);
        pck = dequeuePacket();
    }
}

Uint32 PacketReader::newPacket(QByteArrayView buf)
{
    Uint32 packet_length = 0;
    Uint32 am_of_len_read = 0;
    if (len_received > 0) {
        if (buf.size() < 4 - len_received) {
            memcpy(len + len_received, buf.data(), buf.size());
            len_received += buf.size();
            return buf.size();
        } else {
            memcpy(len + len_received, buf.data(), 4 - len_received);
            am_of_len_read = 4 - len_received;
            len_received = 0;
            packet_length = ReadUint32(len, 0);
        }
    } else if (buf.size() < 4) {
        memcpy(len, buf.data(), buf.size());
        len_received = buf.size();
        return buf.size();
    } else {
        packet_length = ReadUint32(buf, 0);
        am_of_len_read = 4;
    }

    if (packet_length == 0) {
        return am_of_len_read;
    }

    if (packet_length > max_packet_size) {
        Out(SYS_CON | LOG_DEBUG) << " packet_length too large " << packet_length << endl;
        error = true;
        return buf.size();
    }

    packet_queue.emplace_back(IncomingPacket(packet_length));
    return am_of_len_read + readPacket(buf.sliced(am_of_len_read));
}

Uint32 PacketReader::readPacket(QByteArrayView buf)
{
    if (!buf.size()) {
        return 0;
    }

    IncomingPacket &pck = packet_queue.back();
    if (pck.read + buf.size() >= pck.data.size()) {
        // we can read the full packet
        const Uint32 tr = pck.data.size() - pck.read;
        memcpy(pck.data.data() + pck.read, buf.data(), tr);
        pck.read += tr;
        return tr;
    } else {
        // we can do a partial read
        const Uint32 tr = buf.size();
        memcpy(pck.data.data() + pck.read, buf.data(), tr);
        pck.read += tr;
        return tr;
    }
}

void PacketReader::onDataReady(Uint8 *buf_ptr, Uint32 size)
{
    QByteArrayView buf{buf_ptr, size};

    if (error) {
        return;
    }

    const QMutexLocker lock(&mutex);
    if (!packet_queue.empty()) {
        const IncomingPacket &pck = packet_queue.back();
        if (pck.read < pck.data.size()) { // last packet in queue is not fully read
            const auto ret = readPacket(buf);
            buf.slice(ret);
        }
    }

    while (!buf.isEmpty() && !error) {
        const auto ret = newPacket(buf);
        buf.slice(ret);
    }
}
}
