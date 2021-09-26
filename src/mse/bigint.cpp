/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "bigint.h"

#include <QRandomGenerator>

#include <cstring>
#include <util/functions.h>
#include <util/log.h>

using namespace bt;

namespace mse
{
BigInt::BigInt(Uint32 num_bits)
{
    mpz_init2(val, num_bits);
}

BigInt::BigInt(const QString &value)
{
    mpz_init2(val, (value.length() - 2) * 4);
    mpz_set_str(val, value.toLatin1().constData(), 0);
}

BigInt::BigInt(const BigInt &bi)
{
    mpz_set(val, bi.val);
}

BigInt::~BigInt()
{
    mpz_clear(val);
}

BigInt &BigInt::operator=(const BigInt &bi)
{
    mpz_set(val, bi.val);
    return *this;
}

BigInt BigInt::powerMod(const BigInt &x, const BigInt &e, const BigInt &d)
{
    BigInt r;
    mpz_powm(r.val, x.val, e.val, d.val);
    return r;
}

BigInt BigInt::random()
{
    Uint32 tmp[5];
    for (Uint8 i = 0; i < 5; i++)
        tmp[i] = QRandomGenerator::global()->generate();

    return BigInt::fromBuffer(reinterpret_cast<Uint8 *>(tmp), 20);
}

Uint32 BigInt::toBuffer(Uint8 *buf, Uint32 /*max_size*/) const
{
    size_t foo;
    mpz_export(buf, &foo, 1, 1, 1, 0, val);
    return foo;
}

BigInt BigInt::fromBuffer(const Uint8 *buf, Uint32 size)
{
    BigInt r(size * 8);
    mpz_import(r.val, size, 1, 1, 1, 0, buf);
    return r;
}

}
