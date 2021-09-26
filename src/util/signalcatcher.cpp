/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/
#include "signalcatcher.h"
#include "log.h"
#include <klocalizedstring.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef Q_WS_WIN

namespace bt
{
sigjmp_buf sigbus_env;
static bool siglongjmp_safe = false;

static void sigbus_handler(int sig, siginfo_t *siginfo, void *ptr)
{
    Q_UNUSED(siginfo)
    Q_UNUSED(ptr)
    Q_UNUSED(sig)
    // Jump to error handling code,
    // ignore signal if we are not safe to jump
    if (siglongjmp_safe)
        siglongjmp(sigbus_env, 1);
}

static bool InstallBusHandler()
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));

    if (sigaction(SIGBUS, nullptr, &act) != -1 && act.sa_sigaction == sigbus_handler)
        return true;

    act.sa_sigaction = sigbus_handler;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGBUS, &act, nullptr) == -1) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Failed to set SIGBUS handler" << endl;
        return false;
    }

    return true;
}

BusErrorGuard::BusErrorGuard()
{
    InstallBusHandler();
    siglongjmp_safe = true;
}

BusErrorGuard::~BusErrorGuard()
{
    siglongjmp_safe = false;
}

BusError::BusError(bool write_operation)
    : Error(write_operation ? i18n("Error when writing to disk") : i18n("Error when reading from disk"))
    , write_operation(write_operation)
{
}

BusError::~BusError()
{
}

int SignalCatcher::signal_received_pipe[2];

SignalCatcher::SignalCatcher(QObject *parent)
    : QObject(parent)
    , notifier(nullptr)
{
    socketpair(AF_UNIX, SOCK_STREAM, 0, signal_received_pipe);
    notifier = new QSocketNotifier(signal_received_pipe[1], QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &SignalCatcher::handleInput);
}

SignalCatcher::~SignalCatcher()
{
}

void SignalCatcher::signalHandler(int sig, siginfo_t *siginfo, void *ptr)
{
    Q_UNUSED(siginfo);
    Q_UNUSED(ptr);
    ::write(signal_received_pipe[0], &sig, sizeof(int));
}

bool SignalCatcher::catchSignal(int sig)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = SignalCatcher::signalHandler;
    act.sa_flags = SA_SIGINFO;

    if (sigaction(sig, &act, nullptr) == -1) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Failed to set signal handler for " << sig << endl;
        return false;
    }

    return true;
}

void SignalCatcher::handleInput(int fd)
{
    int sig = 0;
    ::read(fd, &sig, sizeof(int));

    Out(SYS_GEN | LOG_IMPORTANT) << "Signal " << sig << " caught " << endl;
    Q_EMIT triggered();
}

}

#endif
