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
#include "functions.h"
#include <qdir.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <gcrypt.h>
#include <qdatetime.h>
#include <QNetworkInterface>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kglobal.h>
#include <interfaces/torrentinterface.h>
#include <util/signalcatcher.h>
#include "error.h"
#include "log.h"

namespace bt
{

	bool IsMultimediaFile(const QString & filename)
	{
		KMimeType::Ptr ptr = KMimeType::findByPath(filename);
		QString name = ptr->name();
		return name.startsWith("audio") || name.startsWith("video") || name == "application/ogg";
	}
	
	QString DirSeparator()
	{
		//QString tmp;
		//tmp.append(QDir::separator());
		return "/";
	}
	
	void UpdateCurrentTime()
	{
		global_time_stamp = Now();
	}
	
	TimeStamp global_time_stamp = 0;
	
	Uint64 Now()
	{
		struct timeval tv;
		gettimeofday(&tv,0);
		global_time_stamp = (Uint64)tv.tv_sec * 1000 + (Uint64)tv.tv_usec * 0.001;
		return global_time_stamp;
	}
	
	Uint32 MaxOpenFiles()
	{
		static Uint32 max_open = 0;
		if (max_open == 0)
		{
			struct rlimit lim;
			getrlimit(RLIMIT_NOFILE,&lim);
			max_open = lim.rlim_cur;
		}
		
		return max_open;
	}
	
	Uint32 CurrentOpenFiles()
	{
		return 0;
		/*
		//return 0;
#ifdef Q_OS_LINUX
		QDir dir(QString("/proc/%1/fd").arg(getpid()));
		return dir.count();
#else
		return 0;
#endif
		*/
	}
	
	bool OpenFileAllowed()
	{
		const Uint32 headroom = 50;
		Uint32 max_open = MaxOpenFiles();
		if (max_open == 0)
			return true;
		else
			return max_open - CurrentOpenFiles() > headroom;
	}

	bool MaximizeLimits()
	{
		// first get the current limits
		struct rlimit lim;
		getrlimit(RLIMIT_NOFILE,&lim);
		
		if (lim.rlim_cur != lim.rlim_max)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Current limit for number of files : " << lim.rlim_cur 
					<< " (" << lim.rlim_max << " max)" << endl;
			lim.rlim_cur = lim.rlim_max;
			if (setrlimit(RLIMIT_NOFILE,&lim) < 0)
			{
				Out(SYS_GEN|LOG_DEBUG) << "Failed to maximize file limit : " 
						<< QString(strerror(errno)) << endl;
				return false;
			}
		}
		else
		{
			Out(SYS_GEN|LOG_DEBUG) << "File limit already at maximum " << endl;
		}
		
		getrlimit(RLIMIT_DATA,&lim);
		if (lim.rlim_cur != lim.rlim_max)
		{
			Out(SYS_GEN|LOG_DEBUG) << "Current limit for data size : " << lim.rlim_cur 
					<< " (" << 	lim.rlim_max << " max)" << endl;
			lim.rlim_cur = lim.rlim_max;
			if (setrlimit(RLIMIT_DATA,&lim) < 0)
			{
				Out(SYS_GEN|LOG_DEBUG) << "Failed to maximize data limit : " 
						<< QString(strerror(errno)) << endl;
				return false;
			}
		}
		else
		{
			Out(SYS_GEN|LOG_DEBUG) << "Data limit already at maximum " << endl;
		}
		
		return true;
	}

	static QString net_iface = QString();
	
	void SetNetworkInterface(const QString & iface)
	{
		net_iface = iface;
	}

	QString NetworkInterface()
	{
		return net_iface;
		
	}
	
	QString NetworkInterfaceIPAddress(const QString & iface)
	{
		QNetworkInterface ni = QNetworkInterface::interfaceFromName(iface);
		if (!ni.isValid())
			return QString();

		QList<QNetworkAddressEntry> addr_list = ni.addressEntries();
		if (addr_list.count() == 0)
			return QString();
		else
			return addr_list.front().ip().toString();
	}
	
	QStringList NetworkInterfaceIPAddresses(const QString& iface)
	{
		QNetworkInterface ni = QNetworkInterface::interfaceFromName(iface);
		if (!ni.isValid())
			return QStringList();
		
		QStringList ips;
		QList<QNetworkAddressEntry> addr_list = ni.addressEntries();
		foreach (const QNetworkAddressEntry & entry,addr_list)
		{
			ips << entry.ip().toString();
		}
		
		return ips;
	}
	
	QString CurrentIPv6Address()
	{
		QNetworkInterface ni = QNetworkInterface::interfaceFromName(net_iface);
		if (!ni.isValid())
		{
			QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
			foreach (const QHostAddress & addr, addrs)
			{
				if (addr.protocol() == QAbstractSocket::IPv6Protocol && addr != QHostAddress::LocalHostIPv6)
					return addr.toString();
			}
		}
		else
		{
			QList<QNetworkAddressEntry> addrs = ni.addressEntries();
			foreach (const QNetworkAddressEntry & entry,addrs)
			{
				QHostAddress addr = entry.ip();
				if (addr.protocol() == QAbstractSocket::IPv6Protocol && addr != QHostAddress::LocalHostIPv6)
					return addr.toString();
			}
		}
		
		return QString();
	}

		
	QString BytesToString(Uint64 bytes)
	{
		KLocale* loc = KGlobal::locale();
		return loc->formatByteSize(bytes,2);
	}

	QString BytesPerSecToString(double speed)
	{
		KLocale* loc = KGlobal::locale();
		return i18n("%1/s",loc->formatByteSize(speed,2));
	}

	QString DurationToString(Uint32 nsecs)
	{
		KLocale* loc = KGlobal::locale();
		QTime t;
		int ndays = nsecs / 86400;
		t = t.addSecs(nsecs % 86400);
		QString s = loc->formatTime(t,true,true);
		if (ndays > 0)
			s = i18np("1 day ","%1 days ",ndays) + s;

		return s;
	}
	
	double Percentage(const TorrentStats & s)
	{
		if (s.bytes_left_to_download == 0)
		{
			return 100.0;
		}
		else
		{
			if (s.total_bytes_to_download == 0)
			{
				return 100.0;
			}
			else
			{
				double perc = 100.0 - ((double)s.bytes_left_to_download / s.total_bytes_to_download) * 100.0;
				if (perc > 100.0)
					perc = 100.0;
				else if (perc > 99.9)
					perc = 99.9;
				else if (perc < 0.0)
					perc = 0.0;
				
				return perc;
			}
		}
	}

#ifdef Q_WS_WIN
	static bool InitWindowsSocketsAPI()
	{
		static bool initialized = false;
		if (initialized)
			return true;

		WSADATA wsaData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		int err = WSAStartup( wVersionRequested, &wsaData );
		if (err != 0) 
			return false;

		initialized = true;
		return true;
	}
#endif

	static bool InitGCrypt()
	{
		static bool initialized = false;
		if (initialized)
			return true;
		
		// If already initialized, don't do anything
		if (gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
		{
			initialized = true;
			return true;
		}
		
		if (!gcry_check_version("1.4.5"))
		{
			Out(SYS_GEN|LOG_NOTICE) << "Failed to initialize libgcrypt" << endl;
			return false;
		}
		/* Disable secure memory. */
		gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
		gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
		initialized = true;
		return true;
	}
	
	bool InitLibKTorrent()
	{
		MaximizeLimits();
		bool ret = InitGCrypt();
#ifdef Q_WS_WIN
		ret = InitWindowsSocketsAPI() && ret;
#endif
		return ret;
	}

}
