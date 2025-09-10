/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTAUTHENTICATEBASE_H
#define BTAUTHENTICATEBASE_H

#include <QObject>
#include <QTimer>
#include <mse/encryptedpacketsocket.h>
#include <util/constants.h>

#include <memory>

namespace bt
{
class SHA1Hash;
class PeerID;

/*!
 * \headerfile peer/authenticatebase.h
 * \author Joris Guisson
 *
 * \brief Base class for carrying out the handshake when connecting to a Peer.
 *
 * This class just groups some common stuff between Authenticate and ServerAuthenticate.
 * It has a socket, handles the timing out, provides a function to send the handshake.
 */
class AuthenticateBase : public QObject
{
    Q_OBJECT
public:
    AuthenticateBase();
    AuthenticateBase(std::unique_ptr<mse::EncryptedPacketSocket> s);
    ~AuthenticateBase() override;

    //! Set whether this is a local peer
    void setLocal(bool loc)
    {
        local = loc;
    }

    //! Is this a local peer
    [[nodiscard]] bool isLocal() const
    {
        return local;
    }

    //! See if the authentication is finished
    [[nodiscard]] bool isFinished() const
    {
        return finished;
    }

    //! Flags indicating which extensions are supported
    [[nodiscard]] Uint32 supportedExtensions() const
    {
        return ext_support;
    }

    //! Take the socket, only if authentication has finished
    std::unique_ptr<mse::EncryptedPacketSocket> takeSocket()
    {
        return finished ? std::move(sock) : nullptr;
    }

    //! get the socket
    [[nodiscard]] mse::EncryptedPacketSocket *getSocket() const
    {
        return sock.get();
    }

    //! We can read from the socket
    virtual void onReadyRead();

    //! We can write to the socket (used to detect a succesfull connection)
    virtual void onReadyWrite();

protected:
    /*!
     * Send a handshake
     * \param info_hash The info_hash to include
     * \param our_peer_id Our PeerID
     */
    void sendHandshake(const SHA1Hash &info_hash, const PeerID &our_peer_id);

    /*!
     * Authentication finished.
     * \param succes Succes or not
     */
    virtual void onFinish(bool succes) = 0;

    /*!
     * The other side send a handshake. The first 20 bytes
     * of the handshake will already have been checked.
     * \param full Indicates whether we have a full handshake
     *  if this is not full, we should just send our own
     */
    virtual void handshakeReceived(bool full) = 0;

    /*!
     * Fill in the handshake in a buffer.
     */
    void makeHandshake(bt::Uint8 *buf, const SHA1Hash &info_hash, const PeerID &our_peer_id);

protected Q_SLOTS:
    void onTimeout();
    void onError(int err);

protected:
    std::unique_ptr<mse::EncryptedPacketSocket> sock;
    QTimer timer;
    bool finished;
    Uint8 handshake[68];
    Uint32 bytes_of_handshake_received;
    Uint32 ext_support;
    bool local;
};

}

#endif
