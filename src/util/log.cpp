/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "log.h"
#include <cstdlib>
#include <iostream>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QTextStream>

#include <KIO/CopyJob>

#include "autorotatelogjob.h"
#include "compressfilejob.h"
#include "error.h"
#include <interfaces/logmonitorinterface.h>
#include <util/fileops.h>

using namespace Qt::Literals::StringLiterals;

namespace bt
{
const Uint32 MAX_LOG_FILE_SIZE = 10 * 1024 * 1024; // 10 MB

static void QtMessageOutput(QtMsgType type, const QMessageLogContext &ctxt, const QString &msg);

class Log::Private
{
public:
    Log *parent;
    QTextStream *out;
    QFile *fptr;
    bool to_cout;
    unsigned int filter;
    QList<LogMonitorInterface *> monitors;
    QString tmp;
    QMutex mutex;
    AutoRotateLogJob *rotate_job;

public:
    Private(Log *parent)
        : parent(parent)
        , out(nullptr)
        , fptr(nullptr)
        , to_cout(false)
        , filter(0)
        , rotate_job(nullptr)
    {
    }

    ~Private()
    {
        cleanup();
    }

    void cleanup()
    {
        delete out;
        out = nullptr;

        delete fptr;
        fptr = nullptr;
    }

    void setFilter(unsigned int f)
    {
        filter = f;
    }

    void rotateLogs(const QString &file)
    {
        if (bt::Exists(file + "-10.gz"_L1))
            bt::Delete(file + "-10.gz"_L1, true);

        // move all log files one up
        for (Uint32 i = 10; i > 1; i--) {
            QString prev = QStringLiteral("%1-%2.gz").arg(file).arg(i - 1);
            QString curr = QStringLiteral("%1-%2.gz").arg(file).arg(i);
            if (bt::Exists(prev))
                QFile::rename(prev, curr);
        }

        // move current log to 1 and zip it
        QFile::rename(file, file + "-1"_L1);
        CompressFileJob *gzip = new CompressFileJob(file + "-1"_L1);
        gzip->start();
    }

    void setOutputFile(const QString &file, bool rotate, bool handle_qt_messages)
    {
        QMutexLocker lock(&mutex);

        if (handle_qt_messages)
            qInstallMessageHandler(QtMessageOutput);

        cleanup();

        if (bt::Exists(file) && rotate)
            rotateLogs(file);

        fptr = new QFile(file);
        if (!fptr->open(QIODevice::WriteOnly)) {
            QString err = fptr->errorString();
            std::cout << "Failed to open log file " << file.toLocal8Bit().constData() << ": " << err.toLocal8Bit().constData() << std::endl;
            cleanup();
            return;
        }

        out = new QTextStream(fptr);
    }

    void write(const QString &line)
    {
        tmp += line;
    }

    void finishLine()
    {
        QString final = QDateTime::currentDateTime().toString() + ": "_L1 + tmp;

        // only add stuff when we are not rotating the logs
        // this could result in the loss of some messages
        if (!rotate_job && fptr != nullptr) {
            if (out)
                *out << final << Qt::endl;

            fptr->flush();
            if (to_cout)
                std::cout << final.toLocal8Bit().constData() << std::endl;
            ;
        }

        if (monitors.count() > 0) {
            QList<LogMonitorInterface *>::iterator i = monitors.begin();
            while (i != monitors.end()) {
                LogMonitorInterface *lmi = *i;
                lmi->message(final, filter);
                ++i;
            }
        }
        tmp.clear();
    }

    void endline()
    {
        finishLine();
        if (fptr && fptr->size() > MAX_LOG_FILE_SIZE && !rotate_job) {
            tmp = QStringLiteral("Log larger then 10 MB, rotating");
            finishLine();
            QString file = fptr->fileName();
            fptr->close(); // close the log file
            out->setDevice(nullptr);
            rotateLogs(file);
            logRotateDone();
        }
    }

    void logRotateDone()
    {
        fptr->open(QIODevice::WriteOnly);
        out->setDevice(fptr);
        rotate_job = nullptr;
    }
};

Log::Log()
    : priv(std::make_unique<Private>(this))
{
}

Log::~Log()
{
    qInstallMessageHandler(nullptr);
}

void Log::setOutputFile(const QString &file, bool rotate, bool handle_qt_messages)
{
    priv->setOutputFile(file, rotate, handle_qt_messages);
}

void Log::addMonitor(LogMonitorInterface *m)
{
    priv->monitors.append(m);
}

void Log::removeMonitor(LogMonitorInterface *m)
{
    int index = priv->monitors.indexOf(m);
    if (index != -1)
        priv->monitors.takeAt(index);
}

void Log::setOutputToConsole(bool on)
{
    priv->to_cout = on;
}

void Log::logRotateDone()
{
    priv->logRotateDone();
}

Log &endl(Log &lg)
{
    lg.priv->endline();
    lg.priv->mutex.unlock(); // unlock after end of line
    return lg;
}

Log &Log::operator<<(const QUrl &url)
{
    return operator<<(url.toString());
}

Log &Log::operator<<(const QString &s)
{
    priv->write(s);
    return *this;
}

Log &Log::operator<<(const char *s)
{
    priv->write(QString::fromUtf8(s));
    return *this;
}

Log &Log::operator<<(Uint64 v)
{
    return operator<<(QString::number(v));
}

Log &Log::operator<<(Int64 v)
{
    return operator<<(QString::number(v));
}

void Log::setFilter(unsigned int filter)
{
    priv->setFilter(filter);
}

void Log::lock()
{
    priv->mutex.lock();
}

Q_GLOBAL_STATIC(Log, global_log)

Log &Out(unsigned int arg)
{
    global_log->setFilter(arg);
    global_log->lock();
    return *global_log;
}

void InitLog(const QString &file, bool rotate, bool handle_qt_messages, bool to_stdout)
{
    global_log->setOutputFile(file, rotate, handle_qt_messages);
    global_log->setOutputToConsole(to_stdout);
}

void AddLogMonitor(LogMonitorInterface *m)
{
    global_log->addMonitor(m);
}

void RemoveLogMonitor(LogMonitorInterface *m)
{
    global_log->removeMonitor(m);
}

static void QtMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
        Out(SYS_GEN | LOG_DEBUG) << "Qt Debug: " << msg << endl;
        break;
    case QtWarningMsg:
    case QtInfoMsg:
        Out(SYS_GEN | LOG_NOTICE) << "Qt Warning: " << msg << endl;
        fprintf(stderr, "Warning: %s\n", msg.toUtf8().constData());
        break;
    case QtCriticalMsg:
        Out(SYS_GEN | LOG_IMPORTANT) << "Qt Critical: " << msg << endl;
        fprintf(stderr, "Critical: %s\n", msg.toUtf8().constData());
        break;
    case QtFatalMsg:
        Out(SYS_GEN | LOG_IMPORTANT) << "Qt Fatal: " << msg << endl;
        fprintf(stderr, "Fatal: %s\n", msg.toUtf8().constData());
        abort();
        break;
    }
}
}
