/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "datacheckerthread.h"
#include "datachecker.h"
#include <torrent/torrent.h>
#include <util/error.h>
#include <util/log.h>

namespace bt
{
DataCheckerThread::DataCheckerThread(std::unique_ptr<DataChecker> dc, const BitSet &status, const QString &path, const Torrent &tor, const QString &dnddir)
    : dc(std::move(dc))
    , path(path)
    , tor(tor)
    , dnddir(dnddir)
    , status(status)
{
    running = true;
    this->dc->moveToThread(this);
}

DataCheckerThread::~DataCheckerThread()
{
}

void DataCheckerThread::run()
{
    try {
        dc->check(path, tor, dnddir, status);
    } catch (bt::Error &e) {
        error = e.toString();
        Out(SYS_GEN | LOG_DEBUG) << error << endl;
    }
    running = false;
}

}
