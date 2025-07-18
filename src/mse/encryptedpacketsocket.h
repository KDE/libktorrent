/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSESTREAMSOCKET_H
#define MSESTREAMSOCKET_H

#include <ktorrent_export.h>
#include <net/packetsocket.h>
#include <util/constants.h>

#include <memory>

class QString;

namespace bt
{
class SHA1Hash;
}

namespace mse
{
class RC4Encryptor;

/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
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

    /*!
     * Send a chunk of data. (Does not encrypt the data)
     * \param data The data
     * \param len The length
     * \return Number of bytes written
     */
    bt::Uint32 sendData(const bt::Uint8 *data, bt::Uint32 len);

    /*!
     * Reads data from the peer.
     * \param buf The buffer to store the data
     * \param len The maximum number of bytes to read
     * \return The number of bytes read
     */
    bt::Uint32 readData(bt::Uint8 *buf, bt::Uint32 len);

    //! Get the number of bytes available to read.
    bt::Uint32 bytesAvailable() const;

    //! Are we using encryption
    bool encrypted() const
    {
        return enc != nullptr;
    }

    /*!
     * Initialize the RC4 encryption algorithm.
     * \param dkey
     * \param ekey
     */
    void initCrypt(const bt::SHA1Hash &dkey, const bt::SHA1Hash &ekey);

    //! Set the encryptor
    void setRC4Encryptor(std::unique_ptr<RC4Encryptor> enc);

    //! Disables encryption. All data will be sent over as plain text.
    void disableCrypt();

    //! Close the socket
    void close();

    //! Connect the socket to a remote host
    bool connectTo(const QString &ip, bt::Uint16 port);

    //! Connect the socket to a remote host
    bool connectTo(const net::Address &addr);

    //! Get the IP address of the remote peer
    QString getRemoteIPAddress() const;

    //! Get the port of the remote peer
    bt::Uint16 getRemotePort() const;

    //! Get the full address
    net::Address getRemoteAddress() const;

    /*!
     * Reinsert data, this is needed when we read to much during the crypto handshake.
     * This data will be the first to read out. The data will be copied to a temporary buffer
     * which will be destroyed when the reinserted data has been read.
     */
    void reinsert(const bt::Uint8 *d, bt::Uint32 size);

    //! see if the socket is still OK
    bool ok() const;

    //! Start monitoring of this socket by the monitor thread
    void startMonitoring(net::SocketReader *rdr);

    //! Stop monitoring this socket
    void stopMonitoring();

    //! Is this socket connecting to a remote host
    bool connecting() const;

    //! See if a connect was success full
    bool connectSuccesFull() const;

    /*!
     * Set the TOS byte for new sockets.
     * \param t TOS value
     */
    static void setTOS(bt::Uint8 t)
    {
        tos = t;
    }

    /*!
     * Set the remote address of the socket. Used by Socks to set the actual
     * address of the connection.
     * \param addr The address
     */
    void setRemoteAddress(const net::Address &addr);

private:
    void preProcess(bt::Uint8 *data, bt::Uint32 size) override;
    void postProcess(bt::Uint8 *data, bt::Uint32 size) override;

private:
    std::unique_ptr<RC4Encryptor> enc;
    bt::Uint8 *reinserted_data;
    bt::Uint32 reinserted_data_size;
    bt::Uint32 reinserted_data_read;
    bool monitored;

    static bt::Uint8 tos;
};

}

#endif
