/*
    SPDX-FileCopyrightText: 2009 Joris Guisson
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
#ifndef KTWAKEUPPIPE_H
#define KTWAKEUPPIPE_H

#include <QMutex>
#include <ktorrent_export.h>
#include <net/poll.h>
#include <util/pipe.h>

namespace net
{
/**
    A WakeUpPipe's purpose is to wakeup a select or poll call.
    It works by using a pipe
    One end needs to be part of the poll or select, and the other end will send dummy data to it.
    Waking up the select or poll call.
*/
class KTORRENT_EXPORT WakeUpPipe : public bt::Pipe, public PollClient
{
public:
    WakeUpPipe();
    ~WakeUpPipe() override;

    /// Wake up the other socket
    virtual void wakeUp();

    /// Read all the dummy data
    void handleData() override;

    int fd() const override
    {
        return readerSocket();
    }

    void reset() override;

    /// Have we been woken up
    bool wokenUp() const
    {
        return woken_up;
    }

    typedef QSharedPointer<WakeUpPipe> Ptr;

protected:
    mutable QMutex mutex;
    bool woken_up;
};

}

#endif
