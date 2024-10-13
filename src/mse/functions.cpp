/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "functions.h"
#include "bigint.h"
#include <util/log.h>
#include <util/sha1hash.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace mse
{
/*
static const BigInt P = BigInt(
        "0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD"
        "129024E088A67CC74020BBEA63B139B22514A08798E3404"
        "DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C"
        "245E485B576625E7EC6F44C42E9A63A36210000000000090563");
*/
static const BigInt P = BigInt(QStringLiteral(
    "0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576"
    "625E7EC6F44C42E9A63A36210000000000090563"));

void GeneratePublicPrivateKey(BigInt &priv, BigInt &pub)
{
    BigInt G = BigInt(u"0x02"_s);
    priv = BigInt::random();
    pub = BigInt::powerMod(G, priv, P);
}

BigInt DHSecret(const BigInt &our_priv, const BigInt &peer_pub)
{
    return BigInt::powerMod(peer_pub, our_priv, P);
}

bt::SHA1Hash EncryptionKey(bool a, const BigInt &s, const bt::SHA1Hash &skey)
{
    Uint8 buf[120];
    memcpy(buf, "key", 3);
    buf[3] = (Uint8)(a ? 'A' : 'B');
    s.toBuffer(buf + 4, 96);
    memcpy(buf + 100, skey.getData(), 20);
    return bt::SHA1Hash::generate(buf, 120);
}

void DumpBigInt(const QString &name, const BigInt &bi)
{
    static Uint8 buf[512];
    Uint32 nb = bi.toBuffer(buf, 512);
    bt::Log &lg = Out(SYS_GEN | LOG_DEBUG);
    lg << name << " (" << nb << ") = ";
    for (Uint32 i = 0; i < nb; i++) {
        lg << u"0x%1 "_s.arg(buf[i], 0, 16);
    }
    lg << endl;
}

}
