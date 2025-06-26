/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSERC4ENCRYPTOR_H
#define MSERC4ENCRYPTOR_H

#if defined(LIBKTORRENT_USE_OPENSSL)
#include <openssl/types.h>
#include <memory>
#elif defined(LIBKTORRENT_USE_LIBGCRYPT)
#include <gcrypt.h>
#endif
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/sha1hash.h>

namespace mse
{
/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * RC4 encryptor. Uses the RC4 algorithm to encrypt and decrypt data.
 * This class has a static encryption buffer, which makes it not thread safe
 * because the buffer is not protected by mutexes.
 */
class KTORRENT_EXPORT RC4Encryptor
{
public:
    RC4Encryptor(const bt::SHA1Hash &dkey, const bt::SHA1Hash &ekey);
    virtual ~RC4Encryptor();

    /*!
     * Decrypt some data, decryption happens in place (original data gets overwritten)
     * \param data The data
     * \param len Size of the data
     */
    void decrypt(bt::Uint8 *data, bt::Uint32 len);

    /*!
     * Encrypt the data. Encryption happens into the static buffer.
     * So that the data passed to this function is never overwritten.
     * If we send pieces we point directly to the mmap region of data,
     * this cannot be overwritten, hence the static buffer.
     * \param data The data
     * \param len The length of the data
     * \return Pointer to the static buffer
     */
    const bt::Uint8 *encrypt(const bt::Uint8 *data, bt::Uint32 len);

    /*!
     * Encrypt data, encryption will happen in the same buffer. So data will
     * be changed replaced by its encrypted version.
     * \param data The data to encrypt
     * \param len The length of the data
     */
    void encryptReplace(bt::Uint8 *data, bt::Uint32 len);

private:
#if defined(LIBKTORRENT_USE_OPENSSL)
    struct EVP_CIPHERDeleter
    {
        void operator()(EVP_CIPHER *cipher) const noexcept;
    };

    struct EVP_CIPHER_CTXDeleter
    {
        void operator()(EVP_CIPHER_CTX *ctx) const noexcept;
    };

    std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTXDeleter> enc;
    std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTXDeleter> dec;
#elif defined(LIBKTORRENT_USE_LIBGCRYPT)
    gcry_cipher_hd_t enc;
    gcry_cipher_hd_t dec;
#endif
};

}

#endif
