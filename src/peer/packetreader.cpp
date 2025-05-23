/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "packetreader.h"
#include "peer.h"
#include <utility>
#include <QtAlgorithms>
#include <util/file.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
IncomingPacket::IncomingPacket(Uint32 size) noexcept
    : size(size)
    , read(0)
{
}

IncomingPacket::Ptr IncomingPacket::create(Uint32 size)
{
    return Ptr(new (size) IncomingPacket(size));
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

IncomingPacket::Ptr PacketReader::dequeuePacket()
{
    QMutexLocker lock(&mutex);
    if (packet_queue.empty())
        return IncomingPacket::Ptr();

    IncomingPacket::Ptr &pck = packet_queue.front();
    if (pck->read != pck->size)
        return IncomingPacket::Ptr();

    IncomingPacket::Ptr p(std::move(pck));
    packet_queue.pop_front();
    return p;
}

void PacketReader::update(PeerInterface &peer)
{
    if (error)
        return;

    IncomingPacket::Ptr pck = dequeuePacket();
    while (pck) {
        peer.handlePacket(pck->data(), pck->size);
        pck = dequeuePacket();
    }
}

Uint32 PacketReader::newPacket(Uint8 *buf, Uint32 size)
{
    Uint32 packet_length = 0;
    Uint32 am_of_len_read = 0;
    if (len_received > 0) {
        if ((int)size < 4 - len_received) {
            memcpy(len + len_received, buf, size);
            len_received += size;
            return size;
        } else {
            memcpy(len + len_received, buf, 4 - len_received);
            am_of_len_read = 4 - len_received;
            len_received = 0;
            packet_length = ReadUint32(len, 0);
        }
    } else if (size < 4) {
        memcpy(len, buf, size);
        len_received = size;
        return size;
    } else {
        packet_length = ReadUint32(buf, 0);
        am_of_len_read = 4;
    }

    if (packet_length == 0)
        return am_of_len_read;

    if (packet_length > max_packet_size) {
        Out(SYS_CON | LOG_DEBUG) << " packet_length too large " << packet_length << endl;
        error = true;
        return size;
    }

    packet_queue.push_back(IncomingPacket::create(packet_length));
    return am_of_len_read + readPacket(buf + am_of_len_read, size - am_of_len_read);
}

Uint32 PacketReader::readPacket(Uint8 *buf, Uint32 size)
{
    if (!size)
        return 0;

    IncomingPacket *pck = packet_queue.back().get();
    if (pck->read + size >= pck->size) {
        // we can read the full packet
        Uint32 tr = pck->size - pck->read;
        memcpy(pck->data() + pck->read, buf, tr);
        pck->read += tr;
        return tr;
    } else {
        // we can do a partial read
        Uint32 tr = size;
        memcpy(pck->data() + pck->read, buf, tr);
        pck->read += tr;
        return tr;
    }
}

void PacketReader::onDataReady(Uint8 *buf, Uint32 size)
{
    if (error)
        return;

    QMutexLocker lock(&mutex);
    Uint32 ret = 0;
    if (!packet_queue.empty()) {
        IncomingPacket *pck = packet_queue.back().get();
        if (pck->read < pck->size) // last packet in queue is not fully read
            ret = readPacket(buf, size);
    }

    while (ret < size && !error) {
        ret += newPacket(buf + ret, size - ret);
    }
}
}
