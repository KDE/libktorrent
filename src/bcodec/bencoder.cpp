/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/

#include "bencoder.h"
#include <QByteArray>
#include <QIODevice>
#include <util/file.h>

namespace bt
{
BEncoderFileOutput::BEncoderFileOutput(File *fptr)
    : fptr(fptr)
{
}

void BEncoderFileOutput::write(const char *str, Uint32 len)
{
    if (fptr)
        fptr->write(str, len);
}

////////////////////////////////////

BEncoderBufferOutput::BEncoderBufferOutput(QByteArray &data)
    : data(data)
    , ptr(0)
{
}

void BEncoderBufferOutput::write(const char *str, Uint32 len)
{
    if (ptr + len > (Uint32)data.size())
        data.resize(ptr + len);

    for (Uint32 i = 0; i < len; i++)
        data[ptr++] = str[i];
}

////////////////////////////////////

BEncoderIODeviceOutput::BEncoderIODeviceOutput(QIODevice *dev)
    : dev(dev)
{
}

void BEncoderIODeviceOutput::write(const char *str, Uint32 len)
{
    dev->write(str, len);
}

////////////////////////////////////

BEncoder::BEncoder(File *fptr)
    : out(nullptr)
    , del(true)
{
    out = new BEncoderFileOutput(fptr);
}

BEncoder::BEncoder(BEncoderOutput *out)
    : out(out)
    , del(true)
{
}

BEncoder::BEncoder(QIODevice *dev)
    : out(nullptr)
    , del(true)
{
    out = new BEncoderIODeviceOutput(dev);
}

BEncoder::~BEncoder()
{
    if (del)
        delete out;
}

void BEncoder::beginDict()
{
    if (!out)
        return;

    out->write("d", 1);
}

void BEncoder::beginList()
{
    if (!out)
        return;

    out->write("l", 1);
}

void BEncoder::write(bool val)
{
    if (!out)
        return;

    QByteArray s = QStringLiteral("i%1e").arg(val ? 1 : 0).toUtf8();
    out->write(s.constData(), s.length());
}

void BEncoder::write(float val)
{
    if (!out)
        return;

    write(QByteArray::number(val, 'f'));
}

void BEncoder::write(Uint32 val)
{
    if (!out)
        return;

    QByteArray s = QStringLiteral("i%1e").arg(val).toUtf8();
    out->write(s.constData(), s.length());
}

void BEncoder::write(Uint64 val)
{
    if (!out)
        return;

    QByteArray s = QStringLiteral("i%1e").arg(val).toUtf8();
    out->write(s.constData(), s.length());
}

void BEncoder::write(const char *str)
{
    if (!out)
        return;

    QByteArray s = QStringLiteral("%1:%2").arg(strlen(str)).arg(QString::fromUtf8(str)).toUtf8();
    out->write(s.constData(), s.length());
}

void BEncoder::write(const QByteArray &data)
{
    if (!out)
        return;

    QByteArray s = QByteArray::number(data.size());
    out->write(s.constData(), s.length());
    out->write(":", 1);
    out->write(data.constData(), data.size());
}

void BEncoder::write(const Uint8 *data, Uint32 size)
{
    if (!out)
        return;

    QByteArray s = QByteArray::number(size);
    out->write(s.constData(), s.length());
    out->write(":", 1);
    out->write((const char *)data, size);
}

void BEncoder::end()
{
    if (!out)
        return;

    out->write("e", 1);
}
}
