/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "datachecker.h"

namespace bt
{
DataChecker::DataChecker(bt::Uint32 from, bt::Uint32 to)
    : need_to_stop(false)
    , from(from)
    , to(to)
{
    failed = found = downloaded = not_downloaded = 0;
}

DataChecker::~DataChecker()
{
}

}

#include "moc_datachecker.cpp"
