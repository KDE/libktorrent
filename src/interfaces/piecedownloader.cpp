/*
    SPDX-FileCopyrightText: 2007 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

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
