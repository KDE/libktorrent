/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETBUFFEREDSOCKET_H
#define NETBUFFEREDSOCKET_H

#include <QMutex>
#include <deque>
#include <download/packet.h>
#include <download/request.h>
#include <net/socket.h>
#include <net/trafficshapedsocket.h>

namespace net
{
/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * Extends the TrafficShapedSocket with outbound bittorrent
 * packet queues.
 */
class PacketSocket : public TrafficShapedSocket
{
public:
    PacketSocket(SocketDevice *sock);
    PacketSocket(int fd, int ip_version);
    PacketSocket(bool tcp, int ip_version);
    ~PacketSocket() override;

    /*!
     * Add a packet to send
     * \param packet The Packet to send
     **/
    void addPacket(bt::Packet::Ptr packet);

    bt::Uint32 write(bt::Uint32 max, bt::TimeStamp now) override;
    bool bytesReadyToWrite() const override;

    //! Get the number of data bytes uploaded
    bt::Uint32 dataBytesUploaded();

    /*!
     * Clear all pieces we are not in the progress of sending.
     * \param reject Whether or not to send a reject
     */
    void clearPieces(bool reject);

    /*!
     * Do not send a piece which matches this request.
     * But only if we are not already sending the piece.
     * \param req The request
     * \param reject Whether we can send a reject instead
     */
    void doNotSendPiece(const bt::Request &req, bool reject);

    //! Get the number of pending piece uploads
    bt::Uint32 numPendingPieceUploads() const;

    //! Get the number of pending piece upload bytes (including message headers)
    bt::Uint32 numPendingPieceUploadBytes() const;

protected:
    /*!
     * Preprocess the packet data, before it is sent. Default implementation does nothing.
     * \param data The packet data
     * \param size The size of the data
     **/
    virtual void preProcess(bt::Uint8 *data, bt::Uint32 size);

private:
    bt::Packet::Ptr selectPacket();

protected:
    std::deque<bt::Packet::Ptr> control_packets;
    std::deque<bt::Packet::Ptr> data_packets; // NOTE: revert back to lists because of erase() calls?
    bt::Packet::Ptr curr_packet;
    bt::Uint32 ctrl_packets_sent;

    bt::Uint32 pending_upload_data_bytes;
    bt::Uint32 uploaded_data_bytes;
};

}

#endif
