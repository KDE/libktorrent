/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "sha1hashgen.h"
#include "functions.h"
#include <QtCrypto>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

namespace bt
{
static QCA::Initializer qca_init;

SHA1HashGen::SHA1HashGen()
    : h(new QCA::Hash(QStringLiteral("sha1")))
{
    memset(result, 9, 20);
}

SHA1HashGen::~SHA1HashGen()
{
    delete h;
}

SHA1Hash SHA1HashGen::generate(const Uint8 *data, Uint32 len)
{
    h->update((const char *)data, len);
    return SHA1Hash((const bt::Uint8 *)h->final().constData());
}

void SHA1HashGen::start()
{
    h->clear();
}

void SHA1HashGen::update(const Uint8 *data, Uint32 len)
{
    h->update((const char *)data, len);
}

void SHA1HashGen::end()
{
    memcpy(result, h->final().constData(), 20);
}

SHA1Hash SHA1HashGen::get() const
{
    return SHA1Hash(result);
}
}
