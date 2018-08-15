/***************************************************************************
 *   Copyright (C) 2010 by Joris Guisson                                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef BT_SIGNALCATCHER_H
#define BT_SIGNALCATCHER_H

#ifndef Q_WS_WIN

#include <ktorrent_export.h>
#include <signal.h>
#include <setjmp.h>
#include <QObject>
#include <QSocketNotifier>
#include <util/error.h>

namespace bt 
{
	/**
		Variable used to jump from the SIGBUS handler back to the place which triggered the SIGBUS.
	*/
	extern KTORRENT_EXPORT sigjmp_buf sigbus_env;

	/**
	 * Protects against SIGBUS errors when doing mmapped IO
	 **/
	class KTORRENT_EXPORT BusErrorGuard
	{
	public:
		BusErrorGuard();
		virtual ~BusErrorGuard();
	};
	
	/**
		Exception throw when a SIGBUS is caught.
	*/
	class KTORRENT_EXPORT BusError : public bt::Error
	{
	public:
		BusError(bool write_operation);
		virtual ~BusError();
		
		/// Wether or not the SIGBUS was triggered by a write operation
		bool write_operation;
	};
	
	/**
	 * Class to handle UNIX signals (not Qt ones)
	 */
	class KTORRENT_EXPORT SignalCatcher : public QObject
	{
		Q_OBJECT
	public:
		SignalCatcher(QObject* parent = 0);
		virtual ~SignalCatcher();
		
		/**
		 * Catch a UNIX signal
		 * @param sig SIGINT, SIGTERM or some other signal
		 * @return true upon success, false otherwise
		 **/
		bool catchSignal(int sig);
		
	private Q_SLOTS:
		void handleInput(int fd);
		
	private:
		static void signalHandler(int sig, siginfo_t *siginfo, void *ptr);
		
	Q_SIGNALS:
		/// Emitted when a 
		void triggered();
		
	private:
		QSocketNotifier* notifier;
		static int signal_received_pipe[2];
	};
}

/// Before writing to memory mapped data, call this macro to ensure that SIGBUS signals are caught and properly dealt with
#define BUS_ERROR_WPROTECT() BusErrorGuard bus_error_guard; if (sigsetjmp(bt::sigbus_env, 1)) throw bt::BusError(true)
	
/// Before reading from memory mapped data, call this macro to ensure that SIGBUS signals are caught and properly dealt with
#define BUS_ERROR_RPROTECT() BusErrorGuard bus_error_guard; if (sigsetjmp(bt::sigbus_env, 1)) throw bt::BusError(false)

#endif

#endif // BT_SIGNALCATCHER_H
