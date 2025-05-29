/*
    SPDX-FileCopyrightText: 2009 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_SERVERINTERFACE_H
#define BT_SERVERINTERFACE_H

#include <QObject>
#include <QStringList>
#include <ktorrent_export.h>
#include <mse/encryptedpacketsocket.h>
#include <util/constants.h>

namespace bt
{
class SHA1Hash;
class PeerManager;

/*!
    Base class for all servers which accept connections.
*/
class KTORRENT_EXPORT ServerInterface : public QObject
{
    Q_OBJECT
public:
    ServerInterface(QObject *parent = nullptr);
    ~ServerInterface() override;

    /*!
     * Change the port.
     * \param port The new port
     */
    virtual bool changePort(Uint16 port) = 0;

    //! Set the port to use
    static void setPort(Uint16 p)
    {
        port = p;
    }

    //! Get the port in use
    static Uint16 getPort()
    {
        return port;
    }

    /*!
     * Add a PeerManager.
     * \param pman The PeerManager
     */
    static void addPeerManager(PeerManager *pman);

    /*!
     * Remove a PeerManager.
     * \param pman The PeerManager
     */
    static void removePeerManager(PeerManager *pman);

    /*!
     * Find the PeerManager given the info_hash of it's torrent.
     * \param hash The info_hash
     * \return The PeerManager or 0 if one can't be found
     */
    static PeerManager *findPeerManager(const SHA1Hash &hash);

    /*!
     * Find the info_hash based on the skey hash. The skey hash is a hash
     * of 'req2' followed by the info_hash. This function finds the info_hash
     * which matches the skey hash.
     * \param skey HASH('req2',info_hash)
     * \param info_hash which matches
     * \return true If one was found
     */
    static bool findInfoHash(const SHA1Hash &skey, SHA1Hash &info_hash);

    /*!
     * Enable encryption.
     * \param allow_unencrypted Allow unencrypted connections (if encryption fails)
     */
    static void enableEncryption(bool allow_unencrypted);

    /*!
     * Disable encrypted authentication.
     */
    static void disableEncryption();

    static bool isEncryptionEnabled()
    {
        return encryption;
    }
    static bool unencryptedConnectionsAllowed()
    {
        return allow_unencrypted;
    }

    /*!
        Get a list of potential IP addresses to bind to
    */
    static QStringList bindAddresses();

    static void setUtpEnabled(bool on, bool only_use_utp);
    static bool isUtpEnabled()
    {
        return utp_enabled;
    }
    static bool onlyUseUtp()
    {
        return only_use_utp;
    }
    static void setPrimaryTransportProtocol(TransportProtocol proto);
    static TransportProtocol primaryTransportProtocol()
    {
        return primary_transport_protocol;
    }

protected:
    void newConnection(mse::EncryptedPacketSocket::Ptr sock);

protected:
    static Uint16 port;
    static QList<PeerManager *> peer_managers;
    static bool encryption;
    static bool allow_unencrypted;
    static bool utp_enabled;
    static bool only_use_utp;
    static TransportProtocol primary_transport_protocol;
};

}

#endif // BT_SERVERINTERFACE_H
