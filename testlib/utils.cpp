/*
    SPDX-FileCopyrightText: 2012 by
    Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "utils.h"

#include <QRandomGenerator64>

bt::Uint64 RandomSize(bt::Uint64 min_size, bt::Uint64 max_size)
{
    bt::Uint64 r = max_size - min_size;
    return min_size + QRandomGenerator64::global()->generate() % r;
}
