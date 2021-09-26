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

#ifndef UTP_UTPSERVERTHREAD_H
#define UTP_UTPSERVERTHREAD_H

#include <QThread>
#include <ktorrent_export.h>

namespace utp
{
class UTPServer;

class KTORRENT_EXPORT UTPServerThread : public QThread
{
    Q_OBJECT
public:
    UTPServerThread(UTPServer *srv);
    ~UTPServerThread() override;

    void run() override;

protected:
    UTPServer *srv;
};

}

#endif // UTP_UTPSERVERTHREAD_H
