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

#include "timevalue.h"
#include <sys/time.h>

namespace utp
{
TimeValue::TimeValue()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    seconds = tv.tv_sec;
    microseconds = tv.tv_usec;
}

TimeValue::TimeValue(bt::Uint64 secs, bt::Uint64 usecs)
    : seconds(secs)
    , microseconds(usecs)
{
}

TimeValue::TimeValue(const utp::TimeValue &tv)
    : seconds(tv.seconds)
    , microseconds(tv.microseconds)
{
}

TimeValue &TimeValue::operator=(const utp::TimeValue &tv)
{
    seconds = tv.seconds;
    microseconds = tv.microseconds;
    return *this;
}

}
