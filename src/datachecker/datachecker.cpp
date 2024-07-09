/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "datachecker.h"

namespace bt
{
DataChecker::DataChecker(bt::Uint32 from, bt::Uint32 to)
    : failed(0)
    , found(0)
    , downloaded(0)
    , not_downloaded(0)
    , need_to_stop(false)
    , from(from)
    , to(to)
{
}

DataChecker::~DataChecker()
{
}

}

#include "moc_datachecker.cpp"
