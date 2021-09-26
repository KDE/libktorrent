/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSESTREAMSOCKET_H
#define MSESTREAMSOCKET_H

#include <ktorrent_export.h>
#include <net/packetsocket.h>
#include <util/constants.h>

class QString;

using bt::Uint16;
using bt::Uint32;
using bt::Uint8;

namespace bt
{
class SHA1Hash;
}

namespace mse
{
class RC4Encryptor;

/**
 * @author Joris Guisson <joris.guisson@gmail.com>
 *
 * Wrapper around a TCP socket which handles RC4 encryption.
 */
class KTORRENT_EXPORT EncryptedPacketSocket : public net::PacketSocket
{
public:
    EncryptedPacketSocket(int ip_version);
    EncryptedPacketSocket(int fd, int ip_version);
    EncryptedPacketSocket(net::SocketDevice *sock);
    ~EncryptedPacketSocket() override;

    /**
     * Send a chunk of data. (Does not encrypt the data)
     * @param data The data
     * @param len The length
     * @return Number of bytes written
     */
    Uint32 sendData(const Uint8 *data, Uint32 len);

    /**
     * Reads data from the peer.
     * @param buf The buffer to store the data
     * @param len The maximum number of bytes to read
     * @return The number of bytes read
     */
    Uint32 readData(Uint8 *buf, Uint32 len);

    /// Get the number of bytes available to read.
    Uint32 bytesAvailable() const;

    /// Are we using encryption
    bool encrypted() const
    {
        return enc != nullptr;
    }

    /**
     * Initialize the RC4 encryption algorithm.
     * @param dkey
     * @param ekey
     */
    void initCrypt(const bt::SHA1Hash &dkey, const bt::SHA1Hash &ekey);

    /// Set the encryptor
    void setRC4Encryptor(RC4Encryptor *enc);

    /// Disables encryption. All data will be sent over as plain text.
    void disableCrypt();

    /// Close the socket
    void close();

    /// Connect the socket to a remote host
    bool connectTo(const QString &ip, Uint16 port);

    /// Connect the socket to a remote host
    bool connectTo(const net::Address &addr);

    /// Get the IP address of the remote peer
    QString getRemoteIPAddress() const;

    /// Get the port of the remote peer
    bt::Uint16 getRemotePort() const;

    /// Get the full address
    net::Address getRemoteAddress() const;

    /**
     * Reinsert data, this is needed when we read to much during the crypto handshake.
     * This data will be the first to read out. The data will be copied to a temporary buffer
     * which will be destroyed when the reinserted data has been read.
     */
    void reinsert(const Uint8 *d, Uint32 size);

    /// see if the socket is still OK
    bool ok() const;

    /// Start monitoring of this socket by the monitor thread
    void startMonitoring(net::SocketReader *rdr);

    /// Stop monitoring this socket
    void stopMonitoring();

    /// Is this socket connecting to a remote host
    bool connecting() const;

    /// See if a connect was success full
    bool connectSuccesFull() const;

    /**
     * Set the TOS byte for new sockets.
     * @param t TOS value
     */
    static void setTOS(Uint8 t)
    {
        tos = t;
    }

    /**
     * Set the remote address of the socket. Used by Socks to set the actual
     * address of the connection.
     * @param addr The address
     */
    void setRemoteAddress(const net::Address &addr);

    typedef QSharedPointer<EncryptedPacketSocket> Ptr;

private:
    void preProcess(bt::Packet::Ptr packet) override;
    void postProcess(Uint8 *data, Uint32 size) override;

private:
    RC4Encryptor *enc;
    Uint8 *reinserted_data;
    Uint32 reinserted_data_size;
    Uint32 reinserted_data_read;
    bool monitored;

    static Uint8 tos;
};

}

#endif
