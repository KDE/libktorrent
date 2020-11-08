/***************************************************************************
 *   Copyright (C) 2008 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
 *   ivasic@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include "chunkselectorinterface.h"
#include <boost/concept_check.hpp>

namespace bt
{

ChunkSelectorInterface::ChunkSelectorInterface() : cman(0), downer(0), pman(0)
{
}


ChunkSelectorInterface::~ChunkSelectorInterface()
{
}

void ChunkSelectorInterface::init(bt::ChunkManager* cman, bt::Downloader* downer, bt::PeerManager* pman)
{
    this->cman = cman;
    this->downer = downer;
    this->pman = pman;
}


bool ChunkSelectorInterface::selectRange(Uint32 & from, Uint32 & to, Uint32 max_len)
{
    Q_UNUSED(from);
    Q_UNUSED(to);
    Q_UNUSED(max_len);
    return false;
}

}
