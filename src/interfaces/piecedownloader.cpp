/*
    SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>

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

#include "moc_piecedownloader.cpp"
