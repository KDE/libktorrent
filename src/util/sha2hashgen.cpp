/*
    SPDX-FileCopyrightText: 2024 Andrius Štikonas <andrius@stikonas.eu>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "sha2hashgen.h"
#include "functions.h"
#include <cstdio>
#include <cstring>

#include <QCryptographicHash>

namespace bt
{

SHA2HashGen::SHA2HashGen()
    : h(new QCryptographicHash(QCryptographicHash::Sha256))
{
    memset(result, 0, 32);
}

SHA2HashGen::~SHA2HashGen()
{
    delete h;
}

SHA2Hash SHA2HashGen::generate(const Uint8 *data, Uint32 len)
{
    h->addData(QByteArrayView((const char *)data, len));
    return SHA2Hash((const bt::Uint8 *)h->result().constData());
}

void SHA2HashGen::start()
{
    h->reset();
}

void SHA2HashGen::update(const Uint8 *data, Uint32 len)
{
    h->addData(QByteArrayView((const char *)data, len));
}

void SHA2HashGen::end()
{
    memcpy(result, h->result().constData(), 32);
}

SHA2Hash SHA2HashGen::get() const
{
    return SHA2Hash(result);
}
}
