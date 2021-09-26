/*
    SPDX-FileCopyrightText: 2007 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "piecedownloader.h"

namespace bt
{
PieceDownloader::PieceDownloader()
    : grabbed(0)
    , nearly_done(false)
{
}

PieceDownloader::~PieceDownloader()
{
}

int PieceDownloader::grab()
{
    grabbed++;
    return grabbed;
}

void PieceDownloader::release()
{
    grabbed--;
    if (grabbed < 0)
        grabbed = 0;
}

}
