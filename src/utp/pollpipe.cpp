/*
    SPDX-FileCopyrightText: 2010 Joris Guisson
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

#include "pollpipe.h"
#include "connection.h"
#include <util/log.h>

using namespace bt;

namespace utp
{
PollPipe::PollPipe(net::Poll::Mode mode)
    : mode(mode)
    , poll_index(-1)
{
}

PollPipe::~PollPipe()
{
}

void PollPipe::reset()
{
    QMutexLocker lock(&mutex);
    poll_index = -1;
    conn_ids.reset();
}

}
