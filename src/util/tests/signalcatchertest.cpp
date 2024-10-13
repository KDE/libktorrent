/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QObject>
#include <QTemporaryFile>
#include <QTest>

#include <csetjmp>
#include <csignal>
#include <ctime>
#include <sys/types.h>
#include <util/fileops.h>
#include <util/log.h>
#include <util/signalcatcher.h>

#ifndef Q_OS_WIN
#include <sys/mman.h>
#include <unistd.h>
#endif

using namespace bt;
using namespace Qt::Literals::StringLiterals;

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
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"signalcatchertest.log"_s);
    }

    void cleanupTestCase()
    {
    }

    void testBus()
    {
        try {
            BUS_ERROR_WPROTECT();
            kill(getpid(), SIGBUS);
            QFAIL("Didn't catch SIGBUS");
        } catch (bt::BusError &e) {
            Out(SYS_GEN | LOG_DEBUG) << u"Caught signal: %1"_s.arg(e.toString()) << endl;
        } catch (...) {
            QFAIL("Didn't catch SIGBUS");
        }
    }

    void testMMap()
    {
        QTemporaryFile tmp;
        QVERIFY(tmp.open());
        int fd = tmp.handle();
        try {
            TruncateFile(fd, 4096, true);
        } catch (bt::Error &err) {
            QString msg = u"Exception thrown: %s"_s.arg(err.toString());
            QFAIL(msg.toLocal8Bit().constData());
        }

        char *ptr = (char *)mmap(nullptr, 4096, PROT_WRITE, MAP_SHARED, fd, 0);
        QVERIFY(ptr);

        // First try a write which should not fail
        try {
            BUS_ERROR_WPROTECT();
            memcpy(ptr, "Testing", 7);
        } catch (bt::BusError &e) {
            QString msg = u"Caught signal: %s"_s.arg(e.toString());
            QFAIL(msg.toLocal8Bit().constData());
        }

        // Lets try one which should fail
        try {
            BUS_ERROR_WPROTECT();
            TruncateFile(fd, 0, true);
            memcpy(ptr, "Testing", 7);
            QFAIL("Didn't catch SIGBUS");
        } catch (bt::BusError &e) {
            Out(SYS_GEN | LOG_DEBUG) << u"Caught signal: %1"_s.arg(e.toString()) << endl;
        }
    }
};

QTEST_MAIN(SignalCatcherTest)

#include "signalcatchertest.moc"
