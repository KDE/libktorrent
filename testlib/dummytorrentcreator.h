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

#ifndef DUMMYTORRENTCREATOR_H
#define DUMMYTORRENTCREATOR_H

#include <QMap>
#include <QStringList>
#include <QTemporaryDir>
#include <util/constants.h>

/**
    Utility class for tests, to create dummy torrents (and their files)
    It will create the torrent and the data in a temporary dir with the following layout:

    tmpdir
     |-> torrent // This is the torrent
     |-> data
          |-> filename.ext // for a single file torrent
          or
          |-> toplevel_dir // for a multifile torrent

    The created files will be filled with random data
*/
class DummyTorrentCreator
{
public:
    DummyTorrentCreator();
    virtual ~DummyTorrentCreator();

    /// Set the tracker URL's (by default http://localhost:5000/announce is used)
    void setTrackers(const QStringList &urls)
    {
        trackers = urls;
    }

    /**
        Create a single file torrent
        @param size The size of the torrent
        @param filename The name of the file
    */
    bool createSingleFileTorrent(bt::Uint64 size, const QString &filename);

    /**
        Create a multi file torrent
        @param files Map of files in the torrent, and their respective sizes
        @param name The name of the torrent (will be used for the toplevel directory name)
    */
    bool createMultiFileTorrent(const QMap<QString, bt::Uint64> &files, const QString &name);

    /// Get the full path of the torrent file
    QString torrentPath() const;

    /// Get the full path of the single data file or toplevel directory (in case of multifile torrents)
    QString dataPath() const;

    /// Get the temp path used
    QString tempPath() const
    {
        return tmpdir.path();
    }

private:
    bool createRandomFile(const QString &path, bt::Uint64 size);

private:
    QTemporaryDir tmpdir;
    QString dpath;

    QStringList trackers;
    bt::Uint32 chunk_size;
};

#endif // DUMMYTORRENTCREATOR_H
