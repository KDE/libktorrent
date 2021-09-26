/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSEFUNCTIONS_H
#define MSEFUNCTIONS_H

#include <QString>

namespace bt
{
class SHA1Hash;
}

namespace mse
{
class BigInt;

void GeneratePublicPrivateKey(BigInt &pub, BigInt &priv);
BigInt DHSecret(const BigInt &our_priv, const BigInt &peer_pub);
bt::SHA1Hash EncryptionKey(bool a, const BigInt &s, const bt::SHA1Hash &skey);

void DumpBigInt(const QString &name, const BigInt &bi);
}

#endif
