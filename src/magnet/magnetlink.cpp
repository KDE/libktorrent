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

#include "magnetlink.h"
#include <QStringList>
#include <QUrlQuery>
#include <util/error.h>
#include <util/log.h>

namespace bt
{
MagnetLink::MagnetLink()
{
}

MagnetLink::MagnetLink(const MagnetLink &mlink)
    : magnet_string(mlink.magnet_string)
    , info_hash(mlink.info_hash)
    , torrent_url(mlink.torrent_url)
    , tracker_urls(mlink.tracker_urls)
    , path(mlink.path)
    , name(mlink.name)
{
}

MagnetLink::MagnetLink(const QUrl &mlink)
{
    parse(mlink);
}

MagnetLink::MagnetLink(const QString &mlink)
{
    parse(QUrl(mlink));
}

MagnetLink::~MagnetLink()
{
}

MagnetLink &MagnetLink::operator=(const bt::MagnetLink &mlink)
{
    magnet_string = mlink.magnet_string;
    info_hash = mlink.info_hash;
    tracker_urls = mlink.tracker_urls;
    torrent_url = mlink.torrent_url;
    path = mlink.path;
    name = mlink.name;
    return *this;
}

bool MagnetLink::operator==(const bt::MagnetLink &mlink) const
{
    return info_hash == mlink.infoHash();
}

static QList<QUrl> GetTrackers(const QUrl &url)
{
    QList<QUrl> result;
    for (QString tr : QUrlQuery(url).allQueryItemValues("tr", QUrl::FullyDecoded))
        result << QUrl(tr.replace(QLatin1Char('+'), QLatin1Char(' ')));
    return result;
}

void MagnetLink::parse(const QUrl &url)
{
    if (url.scheme() != QLatin1String("magnet")) {
        Out(SYS_GEN | LOG_NOTICE) << "Invalid protocol of magnet link " << url << endl;
        return;
    }

    torrent_url = QUrlQuery(url).queryItemValue(QStringLiteral("to"), QUrl::FullyDecoded);
    // magnet://description-of-content.btih.HASH(-HASH)*.dht/path/file?x.pt=&x.to=

    // TODO automatically select these files and prefetches from here
    path = QUrlQuery(url).queryItemValue(QStringLiteral("pt"));
    if (path.isEmpty() && url.path() != QLatin1String("/")) {
        // TODO find out why RemoveTrailingSlash does not work
        path = url.adjusted(QUrl::StripTrailingSlash).path().remove(QRegExp(QLatin1String("^/")));
    }

    QString xt = QUrlQuery(url).queryItemValue(QLatin1String("xt"));
    if (xt.isEmpty() || !xt.startsWith(QLatin1String("urn:btih:"))) {
        QRegExp btihHash(QLatin1String("([^\\.]+).btih"));
        if (btihHash.indexIn(url.host()) != -1) {
            QString primaryHash = btihHash.cap(1).split('-')[0];
            xt = QLatin1String("urn:btih:") + primaryHash;
        } else {
            Out(SYS_GEN | LOG_NOTICE) << "No hash found in magnet link " << url << endl;
            return;
        }
    }

    QString ih = xt.mid(9);
    if (ih.length() != 40 && ih.length() != 32) {
        Out(SYS_GEN | LOG_NOTICE) << "Hash has not valid length in magnet link " << url << endl;
        return;
    }

    try {
        if (ih.length() == 32)
            ih = base32ToHexString(ih);

        Uint8 hash[20];
        memset(hash, 0, 20);
        for (int i = 0; i < 20; i++) {
            Uint8 low = charToHex(ih[2 * i + 1]);
            Uint8 high = charToHex(ih[2 * i]);
            hash[i] = (high << 4) | low;
        }

        info_hash = SHA1Hash(hash);
        tracker_urls = GetTrackers(url);
        name = QUrlQuery(url).queryItemValue(QLatin1String("dn")).replace('+', ' ');
        magnet_string = url.toString();
    } catch (...) {
        Out(SYS_GEN | LOG_NOTICE) << "Invalid magnet link " << url << endl;
    }
}

Uint8 MagnetLink::charToHex(const QChar &ch)
{
    if (ch.isDigit())
        return ch.digitValue();

    if (!ch.isLetter())
        throw bt::Error("Invalid char");

    if (ch.isLower())
        return 10 + ch.toLatin1() - 'a';
    else
        return 10 + ch.toLatin1() - 'A';
}

QString MagnetLink::base32ToHexString(const QString &s)
{
    Uint32 part;
    Uint32 tmp;
    QString ret;
    QChar ch;
    QString str = s.toUpper();
    // 32 base32 chars -> 40 hex chars
    // 4 base32 chars -> 5 hex chars
    for (int i = 0; i < 8; i++) {
        part = 0;
        for (int j = 0; j < 4; j++) {
            ch = str[i * 4 + j];
            if (ch.isDigit() && (ch.digitValue() < 2 || ch.digitValue() > 7))
                throw bt::Error("Invalid char");

            if (ch.isDigit())
                tmp = ch.digitValue() + 24;
            else
                tmp = ch.toLatin1() - 'A';
            part = part + (tmp << 5 * (3 - j));
        }

        // part is a Uint32 with 20 bits (5 hex)
        for (int j = 0; j < 5; j++) {
            tmp = (part >> 4 * (4 - j)) & 0xf;
            if (tmp >= 10)
                ret.append(QChar((tmp - 10) + 'a'));
            else
                ret.append(QChar(tmp + '0'));
        }
    }
    return ret;
}

}
