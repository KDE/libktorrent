/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "chunkselectorinterface.h"

namespace bt
{
ChunkSelectorInterface::ChunkSelectorInterface()
    : cman(nullptr)
    , downer(nullptr)
    , pman(nullptr)
{
}

ChunkSelectorInterface::~ChunkSelectorInterface()
{
}

void ChunkSelectorInterface::init(bt::ChunkManager *cman, bt::Downloader *downer, bt::PeerManager *pman)
{
    this->cman = cman;
    this->downer = downer;
    this->pman = pman;
}

bool ChunkSelectorInterface::selectRange(Uint32 &from, Uint32 &to, Uint32 max_len)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    Q_UNUSED(max_len);
    return false;
}

}
