/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTP_CONNECTION_H
#define UTP_CONNECTION_H

#include <QBasicTimer>
#include <QMutex>
#include <QPair>
#include <QSharedPointer>
#include <QWaitCondition>
#include <ktorrent_export.h>
#include <net/address.h>
#include <util/circularbuffer.h>
#include <util/timer.h>
#include <utp/remotewindow.h>
#include <utp/utpprotocol.h>

namespace utp
{
class DelayWindow;
class LocalWindow;
class Transmitter;

/*!
 * \headerfile utp/connection.h
 * \brief Keeps track of a single UTP connection.
 */
class KTORRENT_EXPORT Connection : public QObject, public Retransmitter
{
    Q_OBJECT
public:
    enum Type {
        INCOMING,
        OUTGOING,
    };

    /*!
     * \headerfile utp/connection.h
     * \brief Thrown when a transmission error occurs, server should kill the connection if it happens.
     */
    class TransmissionError
    {
    public:
        TransmissionError(const char *file, int line);

        QString location;
    };

    /*!
     * \headerfile utp/connection.h
     * \brief Statistics about the Connection.
     */
    struct Stats {
        Type type;
        net::Address remote;
        ConnectionState state;
        bt::Uint16 send_connection_id;
        bt::Uint32 reply_micro;
        bt::Uint16 recv_connection_id;
        bt::Uint16 seq_nr;
        int eof_seq_nr;
        bt::Uint32 timeout;
        TimeValue absolute_timeout;
        int rtt;
        int rtt_var;
        bt::Uint32 packet_size;
        bt::Uint32 last_window_size_transmitted;

        bt::Uint64 bytes_received;
        bt::Uint64 bytes_sent;
        bt::Uint32 packets_received;
        bt::Uint32 packets_sent;
        bt::Uint64 bytes_lost;
        bt::Uint32 packets_lost;

        bool readable;
        bool writeable;
    };

    Connection(bt::Uint16 recv_connection_id, Type type, const net::Address &remote, Transmitter *transmitter);
    ~Connection() override;

    //! Turn on or off blocking mode
    void setBlocking(bool on)
    {
        blocking = on;
    }

    //! Dump connection stats
    void dumpStats();

    //! Start connecting (OUTGOING only)
    void startConnecting();

    //! Get the connection stats
    const Stats &connectionStats() const
    {
        return stats;
    }

    //! Handle a single packet
    ConnectionState handlePacket(const PacketParser &parser, bt::Buffer::Ptr packet);

    //! Get the remote address
    const net::Address &remoteAddress() const
    {
        return stats.remote;
    }

    //! Get the receive connection id
    bt::Uint16 receiveConnectionID() const
    {
        return stats.recv_connection_id;
    }

    //! Send some data, returns the amount of bytes sent (or -1 on error)
    int send(const bt::Uint8 *data, bt::Uint32 len);

    //! Read available data from local window, returns the amount of bytes read
    int recv(bt::Uint8 *buf, bt::Uint32 max_len);

    //! Get the connection state
    ConnectionState connectionState() const
    {
        return stats.state;
    }

    //! Get the type of connection
    Type connectionType() const
    {
        return stats.type;
    }

    //! Get the number of bytes available
    bt::Uint32 bytesAvailable() const;

    //! Can we write to this socket
    bool isWriteable() const;

    //! Wait until the connectTo call fails or succeeds
    bool waitUntilConnected();

    //! Wait until there is data ready or the socket is closed or a timeout occurs
    bool waitForData(bt::Uint32 timeout = 0);

    //! Close the socket
    void close();

    //! Reset the connection
    void reset();

    //! Update the RTT time
    void updateRTT(const Header *hdr, bt::Uint32 packet_rtt, bt::Uint32 packet_size) override;

    //! Retransmit a packet
    void retransmit(PacketBuffer &packet, bt::Uint16 p_seq_nr) override;

    //! Is all data sent
    bool allDataSent() const;

    //! Get the current timeout
    bt::Uint32 currentTimeout() const override
    {
        return stats.timeout;
    }

    using Ptr = QSharedPointer<Connection>;
    using WPtr = QWeakPointer<Connection>;

    //! Set a weak pointer to self
    void setWeakPointer(WPtr ptr)
    {
        self = ptr;
    }

    //! Check if we haven't hit a timeout yet
    void checkTimeout(const TimeValue &now);

private:
    void sendSYN();
    void sendState();
    void sendFIN();
    void sendReset();
    void updateDelayMeasurement(const Header *hdr);
    void sendStateOrData();
    void sendPackets();
    void sendPacket(bt::Uint32 type, bt::Uint16 p_ack_nr);
    void checkIfClosed();
    void sendDataPacket(PacketBuffer &packet, bt::Uint16 seq_nr, const TimeValue &now);
    void startTimer();
    void checkState();
    bt::Uint32 extensionLength() const;
    void handleTimeout();

private:
    Transmitter *transmitter;
    LocalWindow *local_wnd;
    RemoteWindow *remote_wnd;
    bt::CircularBuffer output_buffer;
    // bt::Timer timer;
    mutable QMutex mutex;
    QWaitCondition connected;
    QWaitCondition data_ready;
    Stats stats;
    bool fin_sent;
    TimeValue last_packet_sent;
    DelayWindow *delay_window;
    Connection::WPtr self;
    bool blocking;

    friend class UTPServer;
};

/*!
 * \headerfile utp/connection.h
 * \brief Interface class for transmitting packets and notifying if a connection becomes readable or writeable.
 */
class KTORRENT_EXPORT Transmitter
{
public:
    virtual ~Transmitter();

    //! Send a packet of a connection
    virtual bool sendTo(Connection::Ptr conn, const PacketBuffer &packet) = 0;

    //! Connection has become readable, writeable or both
    virtual void stateChanged(Connection::Ptr conn, bool readable, bool writeable) = 0;

    //! Called when the connection is closed
    virtual void closed(Connection::Ptr conn) = 0;
};

}

#endif // UTP_CONNECTION_H
