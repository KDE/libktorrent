/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSEENCRYPTEDAUTHENTICATE_H
#define MSEENCRYPTEDAUTHENTICATE_H

#include "bigint.h"
#include <peer/authenticate.h>
#include <util/constants.h>
#include <util/sha1hash.h>

#include <memory>

namespace mse
{
class RC4Encryptor;

const bt::Uint32 MAX_EA_BUF_SIZE = 622 + 512;

/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * Encrypted version of the Authenticate class
 */
class EncryptedAuthenticate : public bt::Authenticate
{
    Q_OBJECT
public:
    EncryptedAuthenticate(const net::Address &addr,
                          bt::TransportProtocol proto,
                          const bt::SHA1Hash &info_hash,
                          const bt::PeerID &peer_id,
                          bt::PeerConnector::WPtr pcon,
                          bool we_support_v2);
    ~EncryptedAuthenticate() override;

private Q_SLOTS:
    void connected() override;
    void onReadyRead() override;

private:
    void handleYB();
    void handleCryptoSelect();
    void findVC();
    void handlePadD();

private:
    enum State {
        NOT_CONNECTED,
        SENT_YA,
        GOT_YB,
        FOUND_VC,
        WAIT_FOR_PAD_D,
        NORMAL_HANDSHAKE,
    };

    BigInt xa, ya, s, skey, yb;
    State state;
    std::unique_ptr<RC4Encryptor> our_rc4;
    bt::Uint8 buf[MAX_EA_BUF_SIZE];
    bt::Uint32 buf_size;
    bt::Uint32 vc_off;
    bt::Uint32 dec_bytes;
    bt::SHA1Hash enc, dec;
    bt::Uint32 crypto_select;
    bt::Uint16 pad_D_len;
    bt::Uint32 end_of_crypto_handshake;
};

}

#endif
