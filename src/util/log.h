/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef JORISLOG_H
#define JORISLOG_H

#include "constants.h"
#include <QString>
#include <QUrl>
#include <ktorrent_export.h>

#include <memory>

// LOG MESSAGES CONSTANTS
#define LOG_NONE 0x00
#define LOG_IMPORTANT 0x01
#define LOG_NOTICE 0x03
#define LOG_DEBUG 0x07
#define LOG_ALL 0x0F

#define SYS_GEN 0x0010 // Genereral info messages
#define SYS_CON 0x0020 // Connections
#define SYS_TRK 0x0040 // Tracker
#define SYS_DHT 0x0080 // DHT
#define SYS_DIO 0x0100 // Disk IO related stuff, saving and loading of chunks ...
#define SYS_UTP 0x0200 // UTP

// plugins
#define SYS_IPF 0x1000 // IPFilter
#define SYS_SRC 0x2000 // Search plugin
#define SYS_PNP 0x4000 // UPnP plugin
#define SYS_INW 0x8000 // InfoWidget
#define SYS_SNF 0x10000 // ScanFolder plugin
#define SYS_MPL 0x20000 // Media player plugin
#define SYS_SCD 0x40000 // Scheduler plugin
#define SYS_BTF 0x80000 // BitFinder plugin
#define SYS_WEB 0x100000 // WebInterface plugin
#define SYS_ZCO 0x200000 // ZeroConf plugin
#define SYS_SCR 0x400000 // Scripting plugin
#define SYS_SYN 0x800000 // Syndication plugin

namespace bt
{
class LogMonitorInterface;

/*!
 * \author Joris Guisson
 * \brief Class which writes messages to a logfile
 *
 * This class writes messages to a logfile. To use it, create an instance,
 * set the output file and write stuff with the << operator.
 *
 * By default all messages will also be printed on the standard output. This
 * can be turned down using the \a setOutputToConsole function.
 *
 * There is also the possibility to monitor what is written to the log using
 * the LogMonitorInterface class.
 */
class KTORRENT_EXPORT Log
{
    class Private;

    std::unique_ptr<Private> priv;

public:
    /*!
     * Constructor.
     */
    Log();

    /*!
     * Destructor, closes the file.
     */
    virtual ~Log();

    /*!
     * Enable or disable the printing of log messages to the standard
     * output.
     * \param on Enable or disable
     */
    void setOutputToConsole(bool on);

    /*!
     * Add a log monitor.
     * \param m The log monitor
     */
    void addMonitor(LogMonitorInterface *m);

    /*!
     * Remove a log monitor. It will not be deleted.
     * \param m The log monitor
     */
    void removeMonitor(LogMonitorInterface *m);

    /*!
     * Set the output logfile.
     * \param file The name of the file
     * \param rotate Whether or not to rotate the logs
     * \param handle_qt_messages Whether or not handle Qt messages
     * \throw Exception if the file can't be opened
     */
    void setOutputFile(const QString &file, bool rotate, bool handle_qt_messages);

    /*!
     * Write a number to the log file.
     * Anything which can be passed to QString::number will do.
     * \param val The value
     * \return This Log
     */
    template<class T> Log &operator<<(T val)
    {
        return operator<<(QString::number(val));
    }

    /*!
     * Apply a function to the Log.
     * \param func The function
     * \return This Log
     */
    Log &operator<<(Log &(*func)(Log &))
    {
        return func(*this);
    }

    /*!
     * Output a QString to the log.
     * \param s The QString
     * \return This Log
     */
    Log &operator<<(const char *s);

    /*!
     * Output a QString to the log.
     * \param s The QString
     * \return This Log
     */
    Log &operator<<(const QString &s);

    /*!
     * Output a 64 bit integer to the log.
     * \param v The integer
     * \return This Log
     */
    Log &operator<<(Uint64 v);

    /*!
     * Output a 64 bit integer to the log.
     * \param v The integer
     * \return This Log
     */
    Log &operator<<(Int64 v);

    /*!
     * Prints and endline character to the Log and flushes it.
     * \param lg The Log
     * \return \a lg
     */
    KTORRENT_EXPORT friend Log &endl(Log &lg);

    /*!
     * Write an URL to the file.
     * \param url The QUrl
     * \return This Log
     */
    Log &operator<<(const QUrl &url);

    /*!
     * Sets a filter for log messages. Applies only to listeners via LogMonitorInterface!
     * \param filter SYS & LOG flags combined with bitwise OR.
     */
    void setFilter(unsigned int filter);

    //! Lock the mutex of the log, should be called in Out()
    void lock();

    //! Called by the auto log rotate job when it has finished
    void logRotateDone();
};

KTORRENT_EXPORT Log &endl(Log &lg);
KTORRENT_EXPORT Log &Out(unsigned int arg = 0x00);

/*!
 * Initialize the global log.
 * \param file The log file
 * \param rotate_logs Set to true if the logs need to be rotated
 * \param handle_qt_messages Set to true if Qt messages need to be logged
 * \param to_stdout Set to true if output to standard output is required
 * */
KTORRENT_EXPORT void InitLog(const QString &file, bool rotate_logs = false, bool handle_qt_messages = true, bool to_stdout = false);

//! Add a monitor to the global log
KTORRENT_EXPORT void AddLogMonitor(LogMonitorInterface *m);

//! Remove a monitor from the global log
KTORRENT_EXPORT void RemoveLogMonitor(LogMonitorInterface *m);
}

#endif
