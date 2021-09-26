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

#ifndef UTP_DELAYWINDOW_H
#define UTP_DELAYWINDOW_H

#ifndef Q_MOC_RUN
#include <boost/circular_buffer.hpp>
#endif // Q_MOC_RUN
#include <utp/utpprotocol.h>

namespace utp
{
const bt::Uint32 MAX_DELAY = 0xFFFFFFFF;

class KTORRENT_EXPORT DelayWindow
{
public:
    DelayWindow();
    virtual ~DelayWindow();

    /// Update the window with a new packet, returns the base delay
    bt::Uint32 update(const Header *hdr, bt::TimeStamp receive_time);

private:
    struct DelayEntry {
        bt::Uint32 timestamp_difference_microseconds;
        bt::TimeStamp receive_time;

        DelayEntry()
            : timestamp_difference_microseconds(0)
            , receive_time(0)
        {
        }

        DelayEntry(bt::Uint32 tdm, bt::TimeStamp rt)
            : timestamp_difference_microseconds(tdm)
            , receive_time(rt)
        {
        }

        bool operator<(const DelayEntry &e) const
        {
            return timestamp_difference_microseconds < e.timestamp_difference_microseconds;
        }
    };

    typedef boost::circular_buffer<DelayEntry>::iterator DelayEntryItr;

private:
    boost::circular_buffer<DelayEntry> delay_window;
};

}

#endif // UTP_DELAYWINDOW_H
