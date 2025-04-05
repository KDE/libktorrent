/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#if defined(LIBKTORRENT_USE_OPENSSL)
#include <openssl/types.h>
#include <openssl/params.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <limits.h>
#include <memory>
#elif defined(LIBKTORRENT_USE_LIBGCRYPT)
#include <gcrypt.h>
#endif
#include <QString>
#include "rc4encryptor.h"
#include <util/functions.h>
#include <util/error.h>
#include <util/log.h>

using namespace bt;

namespace mse
{
static Uint8 rc4_enc_buffer[bt::MAX_MSGLEN];

#if defined(LIBKTORRENT_USE_OPENSSL)

inline void RC4Encryptor::EVP_CIPHERDeleter::operator()(EVP_CIPHER *cipher) const noexcept
{
    EVP_CIPHER_free(cipher);
}

inline void RC4Encryptor::EVP_CIPHER_CTXDeleter::operator()(EVP_CIPHER_CTX *ctx) const noexcept
{
    EVP_CIPHER_CTX_free(ctx);
}

RC4Encryptor::RC4Encryptor(const bt::SHA1Hash &dk, const bt::SHA1Hash &ek)
{
    std::unique_ptr<EVP_CIPHER, EVP_CIPHERDeleter> cipher(EVP_CIPHER_fetch(nullptr, "RC4", "provider=legacy"));
    if (!cipher)
        throw bt::Error(QStringLiteral("RC4 cipher not available: ") + getLastOpenSSLErrorString());

    unsigned int key_len = 20;
    OSSL_PARAM params[2] = {
        OSSL_PARAM_construct_uint(OSSL_CIPHER_PARAM_KEYLEN, &key_len),
        OSSL_PARAM_construct_end()
    };

    // The first EVP_EncryptInit_ex2 call sets params *after* initializing the key,
    // so in order to first set the key length we have to call the init twice.
    enc.reset(EVP_CIPHER_CTX_new());
    if (!enc ||
        !EVP_EncryptInit_ex2(enc.get(), cipher.get(), nullptr, nullptr, params) ||
        !EVP_EncryptInit_ex2(enc.get(), nullptr, ek.getData(), nullptr, nullptr))
        throw bt::Error(QStringLiteral("Failed to initialize RC4 encryption context: ") + getLastOpenSSLErrorString());

    dec.reset(EVP_CIPHER_CTX_new());
    if (!dec ||
        !EVP_DecryptInit_ex2(dec.get(), cipher.get(), nullptr, nullptr, params) ||
        !EVP_DecryptInit_ex2(dec.get(), nullptr, dk.getData(), nullptr, nullptr))
        throw bt::Error(QStringLiteral("Failed to initialize RC4 decryption context: ") + getLastOpenSSLErrorString());

    Uint8 tmp[1024];
    int out_len = 0;
    EVP_EncryptUpdate(enc.get(), tmp, &out_len, tmp, 1024);
    EVP_DecryptUpdate(dec.get(), tmp, &out_len, tmp, 1024);
}

RC4Encryptor::~RC4Encryptor() = default;

void RC4Encryptor::decrypt(Uint8 *data, Uint32 len)
{
    int out_len = 0;
    while (len > static_cast<unsigned int>(INT_MAX)) {
        EVP_DecryptUpdate(dec.get(), data, &out_len, data, INT_MAX);
        data += INT_MAX;
        len -= INT_MAX;
    }
    EVP_DecryptUpdate(dec.get(), data, &out_len, data, static_cast<int>(len));
}

const Uint8 *RC4Encryptor::encrypt(const Uint8 *data, Uint32 len)
{
    static_assert(sizeof(rc4_enc_buffer) <= static_cast<unsigned int>(INT_MAX), "rc4_enc_buffer size is too large");
    if (len > sizeof(rc4_enc_buffer))
        throw bt::Error(QStringLiteral("RC4Encryptor::encrypt is called with a too large input: ") + QString::number(len) + QStringLiteral(" bytes"));

    int out_len = 0;
    EVP_EncryptUpdate(enc.get(), rc4_enc_buffer, &out_len, data, static_cast<int>(len));
    return rc4_enc_buffer;
}

void RC4Encryptor::encryptReplace(Uint8 *data, Uint32 len)
{
    int out_len = 0;
    while (len > static_cast<unsigned int>(INT_MAX)) {
        EVP_EncryptUpdate(enc.get(), data, &out_len, data, INT_MAX);
        data += INT_MAX;
        len -= INT_MAX;
    }
    EVP_EncryptUpdate(enc.get(), data, &out_len, data, static_cast<int>(len));
}

#elif defined(LIBKTORRENT_USE_LIBGCRYPT)

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
    if (len > sizeof(rc4_enc_buffer))
        throw bt::Error(QStringLiteral("RC4Encryptor::encrypt is called with a too large input: ") + QString::number(len) + QStringLiteral(" bytes"));
    gcry_cipher_encrypt(enc, rc4_enc_buffer, len, data, len);
    return rc4_enc_buffer;
}

void RC4Encryptor::encryptReplace(Uint8 *data, Uint32 len)
{
    gcry_cipher_encrypt(enc, data, len, data, len);
}

#endif

}
