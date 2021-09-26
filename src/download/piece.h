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
#ifndef BTPIECE_H
#define BTPIECE_H

#include "request.h"

namespace bt
{
/**
@author Joris Guisson
*/
class Piece : public Request
{
public:
    inline Piece(Uint32 index, Uint32 off, Uint32 len, PieceDownloader *pd, const Uint8 *data)
        : Request(index, off, len, pd)
        , data(data)
    {
    }
    ~Piece()
    {
    }

    const Uint8 *getData() const
    {
        return data;
    }

private:
    const Uint8 *data;
};

}

#endif
