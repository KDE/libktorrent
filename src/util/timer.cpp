/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "timer.h"
#include "functions.h"

namespace bt
{
Timer::Timer()
    : elapsed(0)
{
    last = Now();
}

Timer::Timer(const Timer &t)
    : last(t.last)
    , elapsed(t.elapsed)
{
}

Timer::~Timer()
{
}

TimeStamp Timer::update()
{
    TimeStamp now = Now();
    TimeStamp d = (now > last) ? now - last : 0;
    elapsed = d;
    last = now;
    return last;
}

TimeStamp Timer::getElapsedSinceUpdate() const
{
    TimeStamp now = Now();
    return (now > last) ? now - last : 0;
}

Timer &Timer::operator=(const Timer &t)
{
    last = t.last;
    elapsed = t.elapsed;
    return *this;
}
}
