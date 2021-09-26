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
#include "request.h"
#include <interfaces/piecedownloader.h>
#if 0
namespace bt
{
Request::Request() : index(0), off(0), len(0), pd(0)
{}

Request::Request(Uint32 index, Uint32 off, Uint32 len, PieceDownloader* pd)
    : index(index), off(off), len(len), pd(pd)
{}

Request::Request(const Request & r)
    : index(r.index), off(r.off), len(r.len), pd(r.pd)
{}

Request::~Request()
{}


Request & Request::operator = (const Request & r)
{
    index = r.index;
    off = r.off;
    len = r.len;
    pd = r.pd;
    return *this;
}

}
#endif
