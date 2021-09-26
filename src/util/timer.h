/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTTIMER_H
#define BTTIMER_H

#include "constants.h"
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/log.h>

namespace bt
{
/**
@author Joris Guisson
*/
class KTORRENT_EXPORT Timer
{
    TimeStamp last;
    TimeStamp elapsed;

public:
    Timer();
    Timer(const Timer &t);
    virtual ~Timer();

    TimeStamp getLast() const
    {
        return last;
    }
    TimeStamp update();
    TimeStamp getElapsed() const
    {
        return elapsed;
    }
    TimeStamp getElapsedSinceUpdate() const;
    Timer &operator=(const Timer &t);
};
#if 1
class Marker
{
    QString name;
    Timer timer;

public:
    Marker(const QString &name)
        : name(name)
    {
        timer.update();
    }

    void update()
    {
        timer.update();
        Out(SYS_GEN | LOG_DEBUG) << "Mark: " << name << " : " << timer.getElapsed() << " ms" << endl;
    }
};
#endif
}

#endif
