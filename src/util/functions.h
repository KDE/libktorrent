/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTFUNCTIONS_H
#define BTFUNCTIONS_H

#include "constants.h"
#include <QString>
#include <QtEndian>
#include <ktorrent_export.h>

namespace bt
{
struct TorrentStats;

KTORRENT_EXPORT double Percentage(const TorrentStats &s);

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteUint64(Byte *buf, Uint32 off, Uint64 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Uint64 ReadUint64(const Byte *buf, Uint64 off)
{
    return qFromBigEndian<Uint64>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteUint32(Byte *buf, Uint32 off, Uint32 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Uint32 ReadUint32(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Uint32>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteUint16(Byte *buf, Uint32 off, Uint16 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Uint16 ReadUint16(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Uint16>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteInt64(Byte *buf, Uint32 off, Int64 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Int64 ReadInt64(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Int64>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteInt32(Byte *buf, Uint32 off, Int32 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Int32 ReadInt32(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Int32>(buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline void WriteInt16(Byte *buf, Uint32 off, Int16 val)
{
    qToBigEndian(val, buf + off);
}

template<typename Byte>
    requires(sizeof(Byte) == 1)
inline Int16 ReadInt16(const Byte *buf, Uint32 off)
{
    return qFromBigEndian<Int16>(buf + off);
}

KTORRENT_EXPORT void UpdateCurrentTime();

KTORRENT_EXPORT extern TimeStamp global_time_stamp;

inline TimeStamp CurrentTime()
{
    return global_time_stamp;
}

KTORRENT_EXPORT TimeStamp Now();

KTORRENT_EXPORT QString DirSeparator();
KTORRENT_EXPORT bool IsMultimediaFile(const QString &filename);

/*!
 * Maximize the file and memory limits using setrlimit.
 */
KTORRENT_EXPORT bool MaximizeLimits();

//! Get the maximum number of open files
KTORRENT_EXPORT Uint32 MaxOpenFiles();

//! Get the current number of open files
KTORRENT_EXPORT Uint32 CurrentOpenFiles();

//! Can we open another file ?
KTORRENT_EXPORT bool OpenFileAllowed();

//! Set the network interface to use (null means all interfaces)
KTORRENT_EXPORT void SetNetworkInterface(const QString &iface);

//! Get the network interface which needs to be used (this will return the name e.g. eth0, wlan0 ...)
KTORRENT_EXPORT QString NetworkInterface();

//! Get the IP address of the network interface
KTORRENT_EXPORT QString NetworkInterfaceIPAddress(const QString &iface);

//! Get all the IP addresses of the network interface
KTORRENT_EXPORT QStringList NetworkInterfaceIPAddresses(const QString &iface);

//! Get the current IPv6 address
KTORRENT_EXPORT QString CurrentIPv6Address();

const double TO_KB = 1024.0;
const double TO_MEG = (1024.0 * 1024.0);
const double TO_GIG = (1024.0 * 1024.0 * 1024.0);

KTORRENT_EXPORT QString BytesToString(bt::Uint64 bytes);
KTORRENT_EXPORT QString BytesPerSecToString(double speed);
KTORRENT_EXPORT QString DurationToString(bt::Uint32 nsecs);

template<class T>
int CompareVal(T a, T b)
{
    if (a < b) {
        return -1;
    } else if (a > b) {
        return 1;
    } else {
        return 0;
    }
}

template<class T>
QString hex(T val)
{
    return QStringLiteral("0x%1").arg(val, 0, 16);
}

/*!
 * \headerfile util/functions.h
 * \brief Provides access serialization for recursive function calls.
 */
struct KTORRENT_EXPORT RecursiveEntryGuard {
    bool *guard;

    RecursiveEntryGuard(bool *g)
        : guard(g)
    {
        *guard = true;
    }

    ~RecursiveEntryGuard()
    {
        *guard = false;
    }
};

#if defined(LIBKTORRENT_USE_OPENSSL)
//! Returns a string with the last OpenSSL error. Only for internal use in libktorrent for diagnostic purposes.
QString getLastOpenSSLErrorString();
#endif

/*!
    Global initialization function, should be called, in the applications main function.
 */
KTORRENT_EXPORT bool InitLibKTorrent();
}

#endif
