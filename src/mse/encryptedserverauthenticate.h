/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSEENCRYPTEDSERVERAUTHENTICATE_H
#define MSEENCRYPTEDSERVERAUTHENTICATE_H

#include "bigint.h"
#include <peer/serverauthenticate.h>
#include <util/sha1hash.h>

#include <memory>

namespace mse
{
class RC4Encryptor;

const bt::Uint32 MAX_SEA_BUF_SIZE = 608 + 20 + 20 + 8 + 4 + 2 + 512 + 2 + 68;
/*!
    \author Joris Guisson <joris.guisson@gmail.com>
*/
class EncryptedServerAuthenticate : public bt::ServerAuthenticate
{
    Q_OBJECT
public:
    EncryptedServerAuthenticate(mse::EncryptedPacketSocket::Ptr sock);
    ~EncryptedServerAuthenticate() override;

private Q_SLOTS:
    void onReadyRead() override;

private:
    void handleYA();
    void sendYB();
    void findReq1();
    void calculateSKey();
    void processVC();
    void handlePadC();
    void handleIA();

private:
    enum State {
        WAITING_FOR_YA,
        WAITING_FOR_REQ1,
        FOUND_REQ1,
        FOUND_INFO_HASH,
        WAIT_FOR_PAD_C,
        WAIT_FOR_IA,
        NON_ENCRYPTED_HANDSHAKE,
    };
    BigInt xb, yb, s, ya;
    bt::SHA1Hash skey, info_hash;
    State state;
    bt::Uint8 buf[MAX_SEA_BUF_SIZE];
    bt::Uint32 buf_size;
    bt::Uint32 req1_off;
    bt::Uint32 crypto_provide, crypto_select;
    bt::Uint16 pad_C_len;
    bt::Uint16 ia_len;
    std::unique_ptr<RC4Encryptor> our_rc4;
};

}

#endif
