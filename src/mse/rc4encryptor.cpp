/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "rc4encryptor.h"
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace mse
{
static Uint8 rc4_enc_buffer[bt::MAX_MSGLEN];

RC4Encryptor::RC4Encryptor(const bt::SHA1Hash &dk, const bt::SHA1Hash &ek)
{
    gcry_cipher_open(&enc, GCRY_CIPHER_ARCFOUR, GCRY_CIPHER_MODE_STREAM, 0);
    gcry_cipher_setkey(enc, ek.getData(), 20);
    gcry_cipher_open(&dec, GCRY_CIPHER_ARCFOUR, GCRY_CIPHER_MODE_STREAM, 0);
    gcry_cipher_setkey(dec, dk.getData(), 20);

    Uint8 tmp[1024];
    gcry_cipher_encrypt(enc, tmp, 1024, tmp, 1024);
    gcry_cipher_decrypt(dec, tmp, 1024, tmp, 1024);
}

RC4Encryptor::~RC4Encryptor()
{
    gcry_cipher_close(enc);
    gcry_cipher_close(dec);
}

void RC4Encryptor::decrypt(Uint8 *data, Uint32 len)
{
    gcry_cipher_decrypt(dec, data, len, data, len);
}

const Uint8 *RC4Encryptor::encrypt(const Uint8 *data, Uint32 len)
{
    gcry_cipher_encrypt(enc, rc4_enc_buffer, len, data, len);
    return rc4_enc_buffer;
}

void RC4Encryptor::encryptReplace(Uint8 *data, Uint32 len)
{
    gcry_cipher_encrypt(enc, data, len, data, len);
}

}
