/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BT_SIGNALCATCHER_H
#define BT_SIGNALCATCHER_H

#include <utility>

#include <QtSystemDetection>

#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/error.h>

#ifndef Q_OS_WIN
#include <csetjmp>
#include <csignal>

#include <QObject>
#include <QSocketNotifier>
#endif

namespace bt
{
/*!
 * \headerfile util/signalcatcher.h
 * \brief Exception thrown when a SIGBUS is caught.
 */
class KTORRENT_EXPORT BusError : public bt::Error
{
public:
    BusError(bool write_operation);
    ~BusError() override;

    //! Whether or not the SIGBUS was triggered by a write operation
    bool write_operation;
};

#ifndef Q_OS_WIN
/*!
    Variable used to jump from the SIGBUS handler back to the place which triggered the SIGBUS.
*/
extern KTORRENT_EXPORT sigjmp_buf sigbus_env;

/*!
 * \headerfile util/signalcatcher.h
 * \brief Protects against SIGBUS errors when doing mmapped IO.
 */
class KTORRENT_EXPORT BusErrorGuard
{
public:
    BusErrorGuard();
    virtual ~BusErrorGuard();
};

/*!
 * \headerfile util/signalcatcher.h
 * \brief Class to handle UNIX signals (not Qt ones).
 */
class KTORRENT_EXPORT SignalCatcher : public QObject
{
    Q_OBJECT
public:
    SignalCatcher(QObject *parent = nullptr);
    ~SignalCatcher() override;

    /*!
     * Catch a UNIX signal
     * \param sig SIGINT, SIGTERM or some other signal
     * \return true upon success, false otherwise
     **/
    bool catchSignal(int sig);

private Q_SLOTS:
    void handleInput(int fd);

private:
    static void signalHandler(int sig, siginfo_t *siginfo, void *ptr);

Q_SIGNALS:
    //! Emitted when a
    void triggered();

private:
    QSocketNotifier *notifier;
    static int signal_received_pipe[2];
};

#endif // Q_OS_WIN

/*!
 * \enum BusOperation
 *
 * Specifies whether the caller intends to read or write to memory mapped data when calling WithBusErrorProtection.
 *
 * \var Read
 *      Reading from a memory mapped region.
 * \var Write
 *      Writing from a memory mapped region.
 */
enum class BusOperation : Uint8 {
    Read,
    Write,
};

/*!
 * Calls the given \a function with the guarantee that SIGBUS signals are caught and properly dealt with when writing to memory mapped data.
 *
 * In the event that SIGBUS is caught a BusError will be thrown. The \a operation is only used to provide context about the error when the
 * exception is caught. It is not used internally.
 *
 * On Windows this function is a no-op.
 *
 * The return value is propagated like so:
 *
 * \code
 * const auto num_bytes_written = WithBusErrorProtection(BusOperation::Write, [&] {
 *     return file.read(ptr + off, size);
 * });
 * \endcode
 */
template<typename F>
decltype(auto) WithBusErrorProtection(BusOperation operation, F &&function)
{
#ifndef Q_OS_WIN
    BusErrorGuard guard;

    if (sigsetjmp(sigbus_env, 1)) {
        throw BusError(operation == BusOperation::Write);
    }
#endif
    return std::forward<F>(function)();
}
}

#endif // BT_SIGNALCATCHER_H
