/*
    SPDX-FileCopyrightText: 2009 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "remotewindow.h"
#include "connection.h"
#include "utpprotocol.h"
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace utp
{
UnackedPacket::UnackedPacket(const PacketBuffer &packet, bt::Uint16 seq_nr, bt::TimeStamp send_time)
    : packet(packet)
    , seq_nr(seq_nr)
    , send_time(send_time)
    , retransmitted(false)
{
}

UnackedPacket::~UnackedPacket()
{
}

RemoteWindow::RemoteWindow()
    : cur_window(0)
    , max_window(64 * 1024)
    , wnd_size(0)
    , last_ack_nr(0)
    , last_ack_receive_count(0)
{
}

RemoteWindow::~RemoteWindow()
{
    clear();
}

void RemoteWindow::packetReceived(const utp::Header *hdr, const SelectiveAck *sack, Retransmitter *conn)
{
    if (hdr->ack_nr == last_ack_nr) {
        if (hdr->type == ST_STATE)
            last_ack_receive_count++;
    } else {
        last_ack_nr = hdr->ack_nr;
        last_ack_receive_count = 1;
    }

    wnd_size = hdr->wnd_size;

    bt::TimeStamp now = bt::Now();
    QList<UnackedPacket>::iterator i = unacked_packets.begin();
    while (i != unacked_packets.end()) {
        if (SeqNrCmpSE(i->seq_nr, hdr->ack_nr)) {
            // everything up until the ack_nr in the header is acked
            conn->updateRTT(hdr, now - i->send_time, i->packet.payloadSize());
            cur_window -= i->packet.payloadSize();
            i = unacked_packets.erase(i);
        } else if (sack) {
            if (Acked(sack, i->seq_nr - hdr->ack_nr)) {
                conn->updateRTT(hdr, now - i->send_time, i->packet.payloadSize());
                cur_window -= i->packet.payloadSize();
                i = unacked_packets.erase(i);
            } else
                ++i;
        } else
            break;
    }

    if (!unacked_packets.isEmpty()) {
        checkLostPackets(hdr, sack, conn);
    }
}

void RemoteWindow::addPacket(const PacketBuffer &packet, bt::Uint16 seq_nr, bt::TimeStamp send_time)
{
    cur_window += packet.payloadSize();
    wnd_size -= packet.payloadSize();
    unacked_packets.append(UnackedPacket(packet, seq_nr, send_time));
}

void RemoteWindow::checkLostPackets(const utp::Header *hdr, const utp::SelectiveAck *sack, Retransmitter *conn)
{
    bt::TimeStamp now = bt::Now();
    bool lost_packets = false;

    QList<UnackedPacket>::iterator itr = unacked_packets.begin();
    UnackedPacket &first_unacked = *itr;
    if (last_ack_receive_count >= 3 && first_unacked.seq_nr == hdr->ack_nr + 1) {
        // packet has been lost
        if (!first_unacked.retransmitted || now - first_unacked.send_time > conn->currentTimeout()) {
            try {
                conn->retransmit(first_unacked.packet, first_unacked.seq_nr);
                first_unacked.send_time = now;
                first_unacked.retransmitted = true;
            } catch (utp::Connection::TransmissionError &) {
            }
            lost_packets = true;
        }

        ++itr;
    }

    if (sack) {
        bt::Uint16 lost_index = lost(sack);
        while (lost_index > 0 && itr != unacked_packets.end()) {
            bt::Uint16 d = itr->seq_nr - hdr->ack_nr;
            if (d < lost_index && (!itr->retransmitted || now - itr->send_time > conn->currentTimeout())) {
                try {
                    conn->retransmit(itr->packet, itr->seq_nr);
                    itr->send_time = now;
                    itr->retransmitted = true;
                } catch (utp::Connection::TransmissionError &) {
                }
                lost_packets = true;
            }
            ++itr;
        }
    }

    if (lost_packets) {
        Out(SYS_UTP | LOG_DEBUG) << "UTP: lost packets on connection " << hdr->connection_id << endl;
        max_window = (bt::Uint32)qRound(0.78 * max_window);
    }
}

bt::Uint16 RemoteWindow::lost(const SelectiveAck *sack)
{
    // A packet is lost if 3 packets have been acked after it
    bt::Uint32 acked = 0;
    bt::Int16 i = sack->length * 8 - 1;
    while (i >= 0 && acked < 3) {
        if (Acked(sack, i)) {
            acked++;
            if (acked == 3)
                return i;
        }

        i--;
    }

    return 0;
}

void RemoteWindow::timeout(Retransmitter *conn)
{
    try {
        max_window = MIN_PACKET_SIZE;
        bt::TimeStamp now = bt::Now();
        // When a timeout occurs retransmit packets which are lost longer then the current timeout
        for (UnackedPacket &pkt : unacked_packets) {
            if (!pkt.retransmitted || now - pkt.send_time > conn->currentTimeout()) {
                conn->retransmit(pkt.packet, pkt.seq_nr);
                pkt.send_time = bt::Now();
                pkt.retransmitted = true;
            }
        }
    } catch (utp::Connection::TransmissionError &) {
    }
}

void RemoteWindow::updateWindowSize(double scaled_gain)
{
    int d = (int)qRound(scaled_gain);
    if (max_window + d < MIN_PACKET_SIZE)
        max_window = MIN_PACKET_SIZE;
    else
        max_window += d;

    // if (scaled_gain > 1000)
    //  Out(SYS_UTP|LOG_DEBUG) << "RemoteWindow::updateWindowSize " << scaled_gain << " " << max_window << endl;
}

void RemoteWindow::clear()
{
    unacked_packets.clear();
}
}
