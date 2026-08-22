/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BTENDIAN_H
#define BTENDIAN_H

#include <QByteArrayView>
#include <QtEndian>

#include "constants.h"

namespace bt
{
template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteUint64(Byte *buf, Uint32 off, Uint64 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteUint32(Byte *buf, Uint32 off, Uint32 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteUint16(Byte *buf, Uint32 off, Uint16 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteInt64(Byte *buf, Uint32 off, Int64 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteInt32(Byte *buf, Uint32 off, Int32 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteInt16(Byte *buf, Uint32 off, Int16 val)
{
    qToBigEndian(val, buf + off);
}

template<typename T>
    requires(std::is_integral_v<T>)
inline T ReadIntegral(QByteArrayView buf, Uint64 off)
{
    buf.slice(off, sizeof(T));
    return qFromBigEndian<T>(buf.data());
}

inline Uint64 ReadUint64(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Uint64>(buf, off);
}

inline Uint32 ReadUint32(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Uint32>(buf, off);
}

inline Uint16 ReadUint16(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Uint16>(buf, off);
}

inline Uint8 ReadUint8(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Uint8>(buf, off);
}

inline Int64 ReadInt64(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Int64>(buf, off);
}

inline Int32 ReadInt32(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Int32>(buf, off);
}

inline Int16 ReadInt16(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Int16>(buf, off);
}

inline Int8 ReadInt8(QByteArrayView buf, Uint64 off)
{
    return ReadIntegral<Int8>(buf, off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Uint64 ReadUint64(const Byte *buf, Uint64 off)
{
    return qFromBigEndian<Uint64>(buf + off);
}

template<typename Byte>
requires(sizeof(Byte) == 1)
inline Uint32 ReadUint32(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Uint32>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Uint16 ReadUint16(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Uint16>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Int64 ReadInt64(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Int64>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Int32 ReadInt32(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Int32>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Int16 ReadInt16(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Int16>(buf + off);
}
} // namespace bt

#endif // BTENDIAN_H
