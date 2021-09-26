/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTFUNCTIONS_H
#define BTFUNCTIONS_H

#include "constants.h"
#include <QString>
#include <ktorrent_export.h>

namespace bt
{
struct TorrentStats;

KTORRENT_EXPORT double Percentage(const TorrentStats &s);

inline void WriteUint64(Uint8 *buf, Uint32 off, Uint64 val)
{
    buf[off + 0] = (Uint8)((val & 0xFF00000000000000ULL) >> 56);
    buf[off + 1] = (Uint8)((val & 0x00FF000000000000ULL) >> 48);
    buf[off + 2] = (Uint8)((val & 0x0000FF0000000000ULL) >> 40);
    buf[off + 3] = (Uint8)((val & 0x000000FF00000000ULL) >> 32);
    buf[off + 4] = (Uint8)((val & 0x00000000FF000000ULL) >> 24);
    buf[off + 5] = (Uint8)((val & 0x0000000000FF0000ULL) >> 16);
    buf[off + 6] = (Uint8)((val & 0x000000000000FF00ULL) >> 8);
    buf[off + 7] = (Uint8)((val & 0x00000000000000FFULL) >> 0);
}

inline Uint64 ReadUint64(const Uint8 *buf, Uint64 off)
{
    Uint64 tmp = ((Uint64)buf[off] << 56) | ((Uint64)buf[off + 1] << 48) | ((Uint64)buf[off + 2] << 40) | ((Uint64)buf[off + 3] << 32)
        | ((Uint64)buf[off + 4] << 24) | ((Uint64)buf[off + 5] << 16) | ((Uint64)buf[off + 6] << 8) | ((Uint64)buf[off + 7] << 0);

    return tmp;
}

inline void WriteUint32(Uint8 *buf, Uint32 off, Uint32 val)
{
    buf[off + 0] = (Uint8)((val & 0xFF000000) >> 24);
    buf[off + 1] = (Uint8)((val & 0x00FF0000) >> 16);
    buf[off + 2] = (Uint8)((val & 0x0000FF00) >> 8);
    buf[off + 3] = (Uint8)(val & 0x000000FF);
}

inline Uint32 ReadUint32(const Uint8 *buf, Uint32 off)
{
    return (buf[off] << 24) | (buf[off + 1] << 16) | (buf[off + 2] << 8) | buf[off + 3];
}

inline void WriteUint16(Uint8 *buf, Uint32 off, Uint16 val)
{
    buf[off + 0] = (Uint8)((val & 0xFF00) >> 8);
    buf[off + 1] = (Uint8)(val & 0x000FF);
}

inline Uint16 ReadUint16(const Uint8 *buf, Uint32 off)
{
    return (buf[off] << 8) | buf[off + 1];
}

inline void WriteInt64(Uint8 *buf, Uint32 off, Int64 val)
{
    buf[off + 0] = (Uint8)((val & 0xFF00000000000000ULL) >> 56);
    buf[off + 1] = (Uint8)((val & 0x00FF000000000000ULL) >> 48);
    buf[off + 2] = (Uint8)((val & 0x0000FF0000000000ULL) >> 40);
    buf[off + 3] = (Uint8)((val & 0x000000FF00000000ULL) >> 32);
    buf[off + 4] = (Uint8)((val & 0x00000000FF000000ULL) >> 24);
    buf[off + 5] = (Uint8)((val & 0x0000000000FF0000ULL) >> 16);
    buf[off + 6] = (Uint8)((val & 0x000000000000FF00ULL) >> 8);
    buf[off + 7] = (Uint8)((val & 0x00000000000000FFULL) >> 0);
}

inline Int64 ReadInt64(const Uint8 *buf, Uint32 off)
{
    Int64 tmp = ((Int64)buf[off] << 56) | ((Int64)buf[off + 1] << 48) | ((Int64)buf[off + 2] << 40) | ((Int64)buf[off + 3] << 32) | ((Int64)buf[off + 4] << 24)
        | ((Int64)buf[off + 5] << 16) | ((Int64)buf[off + 6] << 8) | ((Int64)buf[off + 7] << 0);

    return tmp;
}

inline void WriteInt32(Uint8 *buf, Uint32 off, Int32 val)
{
    buf[off + 0] = (Uint8)((val & 0xFF000000) >> 24);
    buf[off + 1] = (Uint8)((val & 0x00FF0000) >> 16);
    buf[off + 2] = (Uint8)((val & 0x0000FF00) >> 8);
    buf[off + 3] = (Uint8)(val & 0x000000FF);
}

inline Int32 ReadInt32(const Uint8 *buf, Uint32 off)
{
    return (Int32)(buf[off] << 24) | (buf[off + 1] << 16) | (buf[off + 2] << 8) | buf[off + 3];
}

inline void WriteInt16(Uint8 *buf, Uint32 off, Int16 val)
{
    buf[off + 0] = (Uint8)((val & 0xFF00) >> 8);
    buf[off + 1] = (Uint8)(val & 0x000FF);
}

inline Int16 ReadInt16(const Uint8 *buf, Uint32 off)
{
    return (Int16)(buf[off] << 8) | buf[off + 1];
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

/**
 * Maximize the file and memory limits using setrlimit.
 */
KTORRENT_EXPORT bool MaximizeLimits();

/// Get the maximum number of open files
KTORRENT_EXPORT Uint32 MaxOpenFiles();

/// Get the current number of open files
KTORRENT_EXPORT Uint32 CurrentOpenFiles();

/// Can we open another file ?
KTORRENT_EXPORT bool OpenFileAllowed();

/// Set the network interface to use (null means all interfaces)
KTORRENT_EXPORT void SetNetworkInterface(const QString &iface);

/// Get the network interface which needs to be used (this will return the name e.g. eth0, wlan0 ...)
KTORRENT_EXPORT QString NetworkInterface();

/// Get the IP address of the network interface
KTORRENT_EXPORT QString NetworkInterfaceIPAddress(const QString &iface);

/// Get all the IP addresses of the network interface
KTORRENT_EXPORT QStringList NetworkInterfaceIPAddresses(const QString &iface);

/// Get the current IPv6 address
KTORRENT_EXPORT QString CurrentIPv6Address();

const double TO_KB = 1024.0;
const double TO_MEG = (1024.0 * 1024.0);
const double TO_GIG = (1024.0 * 1024.0 * 1024.0);

KTORRENT_EXPORT QString BytesToString(bt::Uint64 bytes);
KTORRENT_EXPORT QString BytesPerSecToString(double speed);
KTORRENT_EXPORT QString DurationToString(bt::Uint32 nsecs);

template<class T> int CompareVal(T a, T b)
{
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

template<class T> QString hex(T val)
{
    return QStringLiteral("0x%1").arg(val, 0, 16);
}

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

/**
    Global initialization function, should be called, in the applications main function.
 */
KTORRENT_EXPORT bool InitLibKTorrent();
}

#endif
