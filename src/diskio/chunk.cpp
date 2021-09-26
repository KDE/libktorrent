/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "chunk.h"
#include "cache.h"
#include "piecedata.h"
#include <util/sha1hash.h>
#ifndef Q_WS_WIN
#include <util/signalcatcher.h>
#endif

namespace bt
{
Chunk::Chunk(Uint32 index, Uint32 size, Cache *cache)
    : status(Chunk::NOT_DOWNLOADED)
    , index(index)
    , size(size)
    , priority(NORMAL_PRIORITY)
    , cache(cache)
{
}

Chunk::~Chunk()
{
}

bool Chunk::readPiece(Uint32 off, Uint32 len, Uint8 *data)
{
    PieceData::Ptr d = cache->loadPiece(this, off, len);
    if (d && d->ok())
        return d->read(data, len) == len;
    else
        return false;
}

bool Chunk::checkHash(const SHA1Hash &h)
{
    PieceData::Ptr d = getPiece(0, size, true);
    if (!d || !d->ok())
        return false;
    else
        return d->generateHash() == h;
}

PieceData::Ptr Chunk::getPiece(Uint32 off, Uint32 len, bool read_only)
{
    if (read_only)
        return cache->loadPiece(this, off, len);
    else
        return cache->preparePiece(this, off, len);
}

void Chunk::savePiece(PieceData::Ptr piece)
{
    cache->savePiece(piece);
}
}
