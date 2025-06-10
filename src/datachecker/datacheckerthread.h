/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTDATACHECKERTHREAD_H
#define BTDATACHECKERTHREAD_H

#include <QThread>
#include <ktorrent_export.h>
#include <util/bitset.h>

namespace bt
{
class Torrent;
class DataChecker;

/*!
    \author Joris Guisson <joris.guisson@gmail.com>

    \brief Thread which runs a DataChecker.
*/
class KTORRENT_EXPORT DataCheckerThread : public QThread
{
    DataChecker *dc;
    QString path;
    const Torrent &tor;
    QString dnddir;
    bool running;
    QString error;
    BitSet status;

public:
    DataCheckerThread(DataChecker *dc, const BitSet &status, const QString &path, const Torrent &tor, const QString &dnddir);
    ~DataCheckerThread() override;

    void run() override;

    //! Get the data checker
    DataChecker *getDataChecker()
    {
        return dc;
    }

    //! Are we still running
    bool isRunning() const
    {
        return running;
    }

    //! Get the error (if any occurred)
    QString getError() const
    {
        return error;
    }
};

}

#endif
