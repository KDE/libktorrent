/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSERC4ENCRYPTOR_H
#define MSERC4ENCRYPTOR_H

#include <gcrypt.h>
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/sha1hash.h>

using bt::Uint32;
using bt::Uint8;

namespace mse
{
/**
 * @author Joris Guisson <joris.guisson@gmail.com>
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

    /**
     * Decrypt some data, decryption happens in place (original data gets overwritten)
     * @param data The data
     * @param len Size of the data
     */
    void decrypt(Uint8 *data, Uint32 len);

    /**
     * Encrypt the data. Encryption happens into the static buffer.
     * So that the data passed to this function is never overwritten.
     * If we send pieces we point directly to the mmap region of data,
     * this cannot be overwritten, hence the static buffer.
     * @param data The data
     * @param len The length of the data
     * @return Pointer to the static buffer
     */
    const Uint8 *encrypt(const Uint8 *data, Uint32 len);

    /**
     * Encrypt data, encryption will happen in the same buffer. So data will
     * be changed replaced by it's encrypted version.
     * @param data The data to encrypt
     * @param len The length of the data
     */
    void encryptReplace(Uint8 *data, Uint32 len);

private:
    gcry_cipher_hd_t enc;
    gcry_cipher_hd_t dec;
};

}

#endif
