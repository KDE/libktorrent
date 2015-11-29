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

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <ctime>
#include <util/log.h>
#include <util/signalcatcher.h>
#include <util/fileops.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <setjmp.h>


using namespace bt;

/*
static sigjmp_buf sj_env;

static void signal_handler(int sig, siginfo_t *siginfo, void *ptr)
{
	Q_UNUSED(siginfo);
	Q_UNUSED(ptr);
	Q_UNUSED(sig);
	// Jump to error handling code
	siglongjmp(sj_env, 1);
}

bool TestSigBusHandling()
{
	struct sigaction act;
	
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = signal_handler;
	act.sa_flags = SA_SIGINFO;
	
	if (sigaction(SIGBUS, &act, 0) == -1) 
		return false;
	
	if (sigsetjmp(sj_env, 1)) 
	{
		return true;
	}
	
	kill(getpid(), SIGBUS);
	return false;
}
*/

class SignalCatcherTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
		bt::InitLog("signalcatchertest.log");
	}
	
	void cleanupTestCase()
	{
	}
	
	void testBus()
	{
		try
		{
			BUS_ERROR_WPROTECT();
			kill(getpid(), SIGBUS);
			QFAIL("Didn't catch SIGBUS");
		}
		catch (bt::BusError & e)
		{
			Out(SYS_GEN|LOG_DEBUG) << QString("Caught signal: %1").arg(e.toString()) << endl;
		}
		catch (...)
		{
			QFAIL("Didn't catch SIGBUS");
		}
	}

	void testMMap()
	{
		QTemporaryFile tmp;
		QVERIFY(tmp.open());
		int fd = tmp.handle();
		try
		{
			TruncateFile(fd, 4096, true);
		}
		catch (bt::Error & err)
		{
			QString msg = QString("Exception thrown: %s").arg(err.toString());
			QFAIL(msg.toLocal8Bit());
		}
		
		
		char* ptr = (char*)mmap(0, 4096, PROT_WRITE, MAP_SHARED, fd, 0);
		QVERIFY(ptr);
		
		// First try a write which should not fail
		try
		{
			BUS_ERROR_WPROTECT();
			memcpy(ptr,"Testing",7);
		}
		catch (bt::BusError & e)
		{
			QString msg = QString("Caught signal: %s").arg(e.toString());
			QFAIL(msg.toLocal8Bit());
		}
		
		// Lets try one which should fail
		try
		{
			BUS_ERROR_WPROTECT();
			TruncateFile(fd, 0, true);
			memcpy(ptr,"Testing",7);
			QFAIL("Didn't catch SIGBUS");
		}
		catch (bt::BusError & e)
		{
			Out(SYS_GEN|LOG_DEBUG) << QString("Caught signal: %1").arg(e.toString()) << endl;
		}
	}
};

QTEST_MAIN(SignalCatcherTest)

#include "signalcatchertest.moc"
