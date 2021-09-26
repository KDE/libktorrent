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
#include "uploader.h"
#include <diskio/chunkmanager.h>
#include <download/request.h>
#include <peer/peer.h>
#include <peer/peermanager.h>
#include <peer/peeruploader.h>
#include <util/log.h>

namespace bt
{
Uploader::Uploader(ChunkManager &cman, PeerManager &pman)
    : cman(cman)
    , pman(pman)
    , uploaded(0)
{
}

Uploader::~Uploader()
{
}

void Uploader::visit(const bt::Peer::Ptr p)
{
    PeerUploader *pu = p->getPeerUploader();
    uploaded += pu->handleRequests(cman);
}

void Uploader::update()
{
    pman.visit(*this);
}

Uint32 Uploader::uploadRate() const
{
    return pman.uploadRate();
}

}
