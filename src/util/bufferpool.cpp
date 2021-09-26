/*
    SPDX-FileCopyrightText: 2011 Joris Guisson
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

#include "bufferpool.h"

namespace bt
{
Buffer::Buffer(Data data, bt::Uint32 fill, bt::Uint32 cap, QWeakPointer<BufferPool> pool)
    : data(data)
    , fill(fill)
    , cap(cap)
    , pool(pool)
{
}

Buffer::~Buffer()
{
    QSharedPointer<BufferPool> ptr = pool.toStrongRef();
    if (ptr)
        ptr->release(data, cap);
}

BufferPool::BufferPool()
{
}

BufferPool::~BufferPool()
{
}

Buffer::Ptr BufferPool::get(bt::Uint32 min_size)
{
    QMutexLocker lock(&mutex);
    FreeBufferMap::iterator i = free_buffers.lower_bound(min_size);
    if (i != free_buffers.end() && !i->second.empty()) {
        Buffer::Data data = i->second.front();
        i->second.pop_front();
        return Buffer::Ptr(new Buffer(data, min_size, i->first, self));
    } else {
        return Buffer::Ptr(new Buffer(Buffer::Data(new Uint8[min_size]), min_size, min_size, self));
    }
}

void BufferPool::release(Buffer::Data data, bt::Uint32 size)
{
    QMutexLocker lock(&mutex);
    free_buffers[size].push_back(data);
}

void BufferPool::clear()
{
    QMutexLocker lock(&mutex);
    free_buffers.clear();
}

} /* namespace bt */
