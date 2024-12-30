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

SHA2Hash SHA2HashGen::generate(const QByteArrayView data)
{
    h->addData(data);
    return SHA2Hash(h->result());
}

SHA2Hash SHA2HashGen::generate(const Uint8 *data, Uint32 len)
{
    return generate(QByteArrayView(data, len));
}

void SHA2HashGen::start()
{
    h->reset();
}

void SHA2HashGen::update(const QByteArrayView data)
{
    h->addData(data);
}

void SHA2HashGen::update(const Uint8 *data, Uint32 len)
{
    update(QByteArrayView(data, len));
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
