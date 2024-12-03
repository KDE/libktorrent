/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "packetreader.h"
#include "peer.h"
#include <QtAlgorithms>
#include <util/file.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
IncomingPacket::IncomingPacket(Uint32 size)
    : data(new Uint8[size])
    , size(size)
    , read(0)
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

IncomingPacket::Ptr PacketReader::dequeuePacket()
{
    QMutexLocker lock(&mutex);
    if (packet_queue.size() == 0)
        return IncomingPacket::Ptr();

    IncomingPacket::Ptr pck = packet_queue.front();
    if (pck->read != pck->size)
        return IncomingPacket::Ptr();

    packet_queue.pop_front();
    return pck;
}

void PacketReader::update(PeerInterface &peer)
{
    if (error)
        return;

    IncomingPacket::Ptr pck = dequeuePacket();
    while (pck) {
        peer.handlePacket(QByteArrayView(pck->data.data(), pck->size));
        pck = dequeuePacket();
    }
}

Uint32 PacketReader::newPacket(QByteArrayView data)
{
    Uint32 packet_length = 0;
    Uint32 am_of_len_read = 0;
    if (len_received > 0) {
        if (data.size() < 4 - len_received) {
            memcpy(len + len_received, data.constData(), data.size());
            len_received += data.size();
            return data.size();
        } else {
            memcpy(len + len_received, data.constData(), 4 - len_received);
            am_of_len_read = 4 - len_received;
            len_received = 0;
            packet_length = ReadUint32(len, 0);
        }
    } else if (data.size() < 4) {
        memcpy(len, data.constData(), data.size());
        len_received = data.size();
        return data.size();
    } else {
        packet_length = ReadUint32(data, 0);
        am_of_len_read = 4;
    }

    if (packet_length == 0)
        return am_of_len_read;

    if (packet_length > max_packet_size) {
        Out(SYS_CON | LOG_DEBUG) << " packet_length too large " << packet_length << endl;
        error = true;
        return data.size();
    }

    IncomingPacket::Ptr pck(new IncomingPacket(packet_length));
    packet_queue.push_back(pck);
    return am_of_len_read + readPacket(data.sliced(am_of_len_read));
}

Uint32 PacketReader::readPacket(QByteArrayView data)
{
    if (!data.size())
        return 0;

    IncomingPacket::Ptr pck = packet_queue.back();
    if (pck->read + data.size() >= pck->size) {
        // we can read the full packet
        Uint32 tr = pck->size - pck->read;
        memcpy(pck->data.data() + pck->read, data.constData(), tr);
        pck->read += tr;
        return tr;
    } else {
        // we can do a partial read
        Uint32 tr = data.size();
        memcpy(pck->data.data() + pck->read, data.constData(), tr);
        pck->read += tr;
        return tr;
    }
}

void PacketReader::onDataReady(QByteArrayView data)
{
    if (error)
        return;

    mutex.lock();
    if (packet_queue.size() == 0) {
        Uint32 ret = 0;
        while (ret < data.size() && !error) {
            ret += newPacket(data.sliced(ret));
        }
    } else {
        Uint32 ret = 0;
        IncomingPacket::Ptr pck = packet_queue.back();
        if (pck->read == pck->size) // last packet in queue is fully read
            ret = newPacket(data);
        else
            ret = readPacket(data);

        while (ret < data.size() && !error) {
            ret += newPacket(data.sliced(ret));
        }
    }
    mutex.unlock();
}
}
