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
#include "error.h"
#include <util/log.h>

namespace bt
{
Error::Error(const QString &msg)
    : msg(msg)
{
    // Out(SYS_GEN|LOG_DEBUG) << "Error thrown: " << msg << endl;
}

Error::~Error()
{
}

Warning::Warning(const QString &msg)
    : msg(msg)
{
    // Out(SYS_GEN|LOG_DEBUG) << "Warning thrown: " << msg << endl;
}

Warning::~Warning()
{
}
}
