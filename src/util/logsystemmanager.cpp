/*
    SPDX-FileCopyrightText: 2008 Joris Guisson and Ivan Vasic
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
#include "logsystemmanager.h"
#include "log.h"
#include <klocalizedstring.h>

namespace bt
{
QScopedPointer<LogSystemManager> LogSystemManager::self;

LogSystemManager::LogSystemManager()
    : QObject()
{
    // register default systems
    registerSystem(i18n("General"), SYS_GEN);
    registerSystem(i18n("Connections"), SYS_CON);
    registerSystem(i18n("Tracker"), SYS_TRK);
    registerSystem(i18n("DHT"), SYS_DHT);
    registerSystem(i18n("Disk Input/Output"), SYS_DIO);
    registerSystem(i18n("µTP"), SYS_UTP);
}

LogSystemManager::~LogSystemManager()
{
}

LogSystemManager &LogSystemManager::instance()
{
    if (!self)
        self.reset(new LogSystemManager());
    return *self;
}

void LogSystemManager::registerSystem(const QString &name, Uint32 id)
{
    systems.insert(name, id);
    registered(name);
}

void LogSystemManager::unregisterSystem(const QString &name)
{
    if (systems.remove(name))
        unregisted(name);
}

Uint32 LogSystemManager::systemID(const QString &name)
{
    iterator i = systems.find(name);
    if (i == systems.end())
        return 0;
    else
        return i.value();
}

}
