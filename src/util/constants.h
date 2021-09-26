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
#ifndef BTCONSTANTS_H
#define BTCONSTANTS_H

#include <QtGlobal>

namespace bt
{
typedef quint64 Uint64;
typedef quint32 Uint32;
typedef quint16 Uint16;
typedef quint8 Uint8;

typedef qint64 Int64;
typedef qint32 Int32;
typedef qint16 Int16;
typedef qint8 Int8;

typedef Uint64 TimeStamp;

typedef enum {
    // also leave some room if we want to add new priorities in the future
    FIRST_PREVIEW_PRIORITY = 55,
    FIRST_PRIORITY = 50,
    NORMAL_PREVIEW_PRIORITY = 45,
    NORMAL_PRIORITY = 40,
    LAST_PREVIEW_PRIORITY = 35,
    LAST_PRIORITY = 30,
    ONLY_SEED_PRIORITY = 20,
    EXCLUDED = 10,
} Priority;

enum ConfirmationResult {
    KEEP_DATA,
    THROW_AWAY_DATA,
    CANCELED,
};

const Uint32 MAX_MSGLEN = 9 + 131072;
const Uint16 MIN_PORT = 6881;
const Uint16 MAX_PORT = 6889;
const Uint32 MAX_PIECE_LEN = 16384;

const Uint8 CHOKE = 0;
const Uint8 UNCHOKE = 1;
const Uint8 INTERESTED = 2;
const Uint8 NOT_INTERESTED = 3;
const Uint8 HAVE = 4;
const Uint8 BITFIELD = 5;
const Uint8 REQUEST = 6;
const Uint8 PIECE = 7;
const Uint8 CANCEL = 8;
const Uint8 PORT = 9;
const Uint8 SUGGEST_PIECE = 13;
const Uint8 HAVE_ALL = 14;
const Uint8 HAVE_NONE = 15;
const Uint8 REJECT_REQUEST = 16;
const Uint8 ALLOWED_FAST = 17;
const Uint8 EXTENDED = 20; // extension protocol message

// flags for things which a peer supports
const Uint32 DHT_SUPPORT = 0x01;
const Uint32 EXT_PROT_SUPPORT = 0x10;
const Uint32 FAST_EXT_SUPPORT = 0x04;

enum TransportProtocol { TCP, UTP };
}

#endif
