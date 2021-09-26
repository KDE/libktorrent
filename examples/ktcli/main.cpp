/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ktcli.h"
#include <QCommandLineParser>
#include <QDir>
#include <util/error.h>
#include <util/functions.h>
#include <util/log.h>
#include <version.h>

#ifndef Q_WS_WIN
#include <signal.h>

void signalhandler(int sig)
{
    if (sig == SIGINT) {
        qApp->quit();
    }
}

#endif

using namespace bt;

int main(int argc, char **argv)
{
#ifndef Q_WS_WIN
    signal(SIGINT, signalhandler);
#endif
    try {
        if (!bt::InitLibKTorrent()) {
            fprintf(stderr, "Failed to initialize libktorrent\n");
            return -1;
        }

        QCoreApplication::setApplicationName("ktcli");
        QCoreApplication::setApplicationVersion(bt::GetVersionString());

        KTCLI app(argc, argv);
        if (!app.start())
            return -1;
        else
            return app.exec();
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Uncaught error: " << err.toString() << endl;
    } catch (std::exception &err) {
        Out(SYS_GEN | LOG_IMPORTANT) << "Uncaught error: " << err.what() << endl;
    }

    return -1;
}
