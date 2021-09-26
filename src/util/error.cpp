/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "error.h"
#include <util/log.h>

namespace bt
{
Error::Error(const QString &msg)
    : msg(msg)
{
    // Out(SYS_GEN|LOG_DEBUG) << "Error thrown: " << msg << endl;
}

Error::~Error()
{
}

Warning::Warning(const QString &msg)
    : msg(msg)
{
    // Out(SYS_GEN|LOG_DEBUG) << "Warning thrown: " << msg << endl;
}

Warning::~Warning()
{
}
}
