/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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

void BEncoderFileOutput::write(QByteArrayView str)
{
    if (fptr) {
        fptr->write(str.data(), str.size());
    }
}

////////////////////////////////////

BEncoderBufferOutput::BEncoderBufferOutput(QByteArray &data)
    : data(data)
{
    // Using resize here since clear() will destroy the array's internal buffer and create a new one. resize() just sets the size property to 0, re-using the
    // old buffer.
    data.resize(0);
}

void BEncoderBufferOutput::write(QByteArrayView str)
{
    data.append(str);
}

////////////////////////////////////

BEncoderIODeviceOutput::BEncoderIODeviceOutput(QIODevice *dev)
    : dev(dev)
{
}

void BEncoderIODeviceOutput::write(QByteArrayView str)
{
    dev->write(str.data(), str.size());
}

////////////////////////////////////

BEncoder::BEncoder(File *fptr)
    : out(std::make_unique<BEncoderFileOutput>(fptr))
{
}

BEncoder::BEncoder(std::unique_ptr<BEncoderOutput> out)
    : out(std::move(out))
{
}

BEncoder::BEncoder(QIODevice *dev)
    : out(std::make_unique<BEncoderIODeviceOutput>(dev))
{
}

BEncoder::~BEncoder()
{
}

void BEncoder::beginDict()
{
    if (!out) {
        return;
    }

    out->write("d");
}

void BEncoder::beginList()
{
    if (!out) {
        return;
    }

    out->write("l");
}

void BEncoder::write(bool val)
{
    if (!out) {
        return;
    }

    QByteArray s = QStringLiteral("i%1e").arg(val ? 1 : 0).toUtf8();
    out->write(s);
}

void BEncoder::write(float val)
{
    if (!out) {
        return;
    }

    write(QByteArray::number(val, 'f'));
}

void BEncoder::write(Uint32 val)
{
    if (!out) {
        return;
    }

    QByteArray s = QStringLiteral("i%1e").arg(val).toUtf8();
    out->write(s);
}

void BEncoder::write(Uint64 val)
{
    if (!out) {
        return;
    }

    QByteArray s = QStringLiteral("i%1e").arg(val).toUtf8();
    out->write(s);
}

void BEncoder::write(QByteArrayView data)
{
    if (!out) {
        return;
    }

    QByteArray s = QByteArray::number(data.size());
    out->write(s);
    out->write(":");
    out->write(data);
}

void BEncoder::end()
{
    if (!out) {
        return;
    }

    out->write("e");
}
}
