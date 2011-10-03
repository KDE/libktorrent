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
#ifndef BTFUNCTIONS_H
#define BTFUNCTIONS_H


#include <ktorrent_export.h>
#include <QString>
#include "constants.h"


namespace bt
{
	struct TorrentStats;
	
	KTORRENT_EXPORT double Percentage(const TorrentStats & s);

	KTORRENT_EXPORT void WriteUint64(Uint8* buf,Uint32 off,Uint64 val);
	KTORRENT_EXPORT Uint64 ReadUint64(const Uint8* buf,Uint64 off);
	
	KTORRENT_EXPORT void WriteUint32(Uint8* buf,Uint32 off,Uint32 val);
	KTORRENT_EXPORT Uint32 ReadUint32(const Uint8* buf,Uint32 off);
	
	KTORRENT_EXPORT void WriteUint16(Uint8* buf,Uint32 off,Uint16 val);
	KTORRENT_EXPORT Uint16 ReadUint16(const Uint8* buf,Uint32 off);

	
	KTORRENT_EXPORT void WriteInt64(Uint8* buf,Uint32 off,Int64 val);
	KTORRENT_EXPORT Int64 ReadInt64(const Uint8* buf,Uint32 off);
	
	KTORRENT_EXPORT void WriteInt32(Uint8* buf,Uint32 off,Int32 val);
	KTORRENT_EXPORT Int32 ReadInt32(const Uint8* buf,Uint32 off);
	
	KTORRENT_EXPORT void WriteInt16(Uint8* buf,Uint32 off,Int16 val);
	KTORRENT_EXPORT Int16 ReadInt16(const Uint8* buf,Uint32 off);
	
	KTORRENT_EXPORT void UpdateCurrentTime();
	
	KTORRENT_EXPORT extern TimeStamp global_time_stamp;
	
	inline TimeStamp CurrentTime() {return global_time_stamp;}
	
	KTORRENT_EXPORT TimeStamp Now();
	
	KTORRENT_EXPORT QString DirSeparator();
	KTORRENT_EXPORT bool IsMultimediaFile(const QString & filename);

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
	KTORRENT_EXPORT void SetNetworkInterface(const QString & iface);
	
	/// Get the network interface which needs to be used (this will return the name e.g. eth0, wlan0 ...)
	KTORRENT_EXPORT QString NetworkInterface(); 

	/// Get the IP address of the network interface
	KTORRENT_EXPORT QString NetworkInterfaceIPAddress(const QString & iface);
	
	/// Get all the IP addresses of the network interface
	KTORRENT_EXPORT QStringList NetworkInterfaceIPAddresses(const QString & iface);
	
	const double TO_KB = 1024.0;
	const double TO_MEG = (1024.0 * 1024.0);
	const double TO_GIG = (1024.0 * 1024.0 * 1024.0);
	
	KTORRENT_EXPORT QString BytesToString(bt::Uint64 bytes);
	KTORRENT_EXPORT QString BytesPerSecToString(double speed);
	KTORRENT_EXPORT QString DurationToString(bt::Uint32 nsecs);
	
	template<class T> int CompareVal(T a,T b)
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
		return QString("0x%1").arg(val,0,16);
	}
	
	struct KTORRENT_EXPORT RecursiveEntryGuard
	{
		bool* guard;
		
		RecursiveEntryGuard(bool* g) : guard(g)
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
