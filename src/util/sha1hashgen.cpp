/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "sha1hashgen.h"
#include "functions.h"
#include <cstdio>
#include <cstring>

#include <QCryptographicHash>

namespace bt
{

SHA1HashGen::SHA1HashGen()
    : h(new QCryptographicHash(QCryptographicHash::Sha1))
{
    memset(result, 9, 20);
}

SHA1HashGen::~SHA1HashGen()
{
    delete h;
}

SHA1Hash SHA1HashGen::generate(const QByteArrayView data)
{
    h->addData(data);
    return SHA1Hash(h->result());
}

SHA1Hash SHA1HashGen::generate(const Uint8 *data, Uint32 len)
{
    return generate(QByteArrayView(data, len));
}

void SHA1HashGen::start()
{
    h->reset();
}

void SHA1HashGen::update(const QByteArrayView data)
{
    h->addData(data);
}

void SHA1HashGen::update(const Uint8 *data, Uint32 len)
{
    update(QByteArrayView(data, len));
}

void SHA1HashGen::end()
{
    memcpy(result, h->result().constData(), 20);
}

SHA1Hash SHA1HashGen::get() const
{
    return SHA1Hash(result);
}
}
