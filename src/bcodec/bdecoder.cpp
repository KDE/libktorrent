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
#include "bdecoder.h"
#include "bnode.h"
#include <klocalizedstring.h>
#include <util/error.h>
#include <util/log.h>

namespace bt
{
BDecoder::BDecoder(const Uint8 *ptr, Uint32 size, bool verbose, Uint32 off)
    : data(QByteArray::fromRawData((const char *)ptr, size))
    , pos(off)
    , verbose(verbose)
    , level(0)
{
}

BDecoder::BDecoder(const QByteArray &data, bool verbose, Uint32 off)
    : data(data)
    , pos(off)
    , verbose(verbose)
    , level(0)
{
}

BDecoder::~BDecoder()
{
}

BNode *BDecoder::decode()
{
    if (pos >= (Uint32)data.size())
        return nullptr;

    if (data[pos] == 'd') {
        return parseDict();
    } else if (data[pos] == 'l') {
        return parseList();
    } else if (data[pos] == 'i') {
        return parseInt();
    } else if (data[pos] >= '0' && data[pos] <= '9') {
        return parseString();
    } else {
        throw Error(i18n("Illegal token: %1", data[pos]));
    }
}

BDictNode *BDecoder::decodeDict()
{
    BNode *n = nullptr;
    try {
        n = decode();
        if (n && n->getType() == BNode::DICT)
            return (BDictNode *)n;

        delete n;
    } catch (...) {
        delete n;
        throw;
    }

    return nullptr;
}

BListNode *BDecoder::decodeList()
{
    BNode *n = nullptr;
    try {
        n = decode();
        if (n && n->getType() == BNode::LIST)
            return (BListNode *)n;

        delete n;
    } catch (...) {
        delete n;
        throw;
    }

    return nullptr;
}

BDictNode *BDecoder::parseDict()
{
    Uint32 off = pos;
    // we're now entering a dictionary
    BDictNode *curr = new BDictNode(off);
    pos++;
    debugMsg(QString("DICT"));
    level++;
    try {
        while (pos < (Uint32)data.size() && data[pos] != 'e') {
            debugMsg(QString("Key : "));
            BNode *kn = decode();
            BValueNode *k = dynamic_cast<BValueNode *>(kn);
            if (!k || k->data().getType() != Value::STRING) {
                delete kn;
                throw Error(i18n("Decode error"));
            }

            QByteArray key = k->data().toByteArray();
            delete kn;

            BNode *value = decode();
            if (!value)
                throw Error(i18n("Decode error"));

            curr->insert(key, value);
        }
        pos++;
    } catch (...) {
        delete curr;
        throw;
    }
    level--;
    debugMsg(QString("END"));
    curr->setLength(pos - off);
    return curr;
}

BListNode *BDecoder::parseList()
{
    Uint32 off = pos;
    debugMsg(QString("LIST"));
    level++;
    BListNode *curr = new BListNode(off);
    pos++;
    try {
        while (pos < (Uint32)data.size() && data[pos] != 'e') {
            BNode *n = decode();
            if (n)
                curr->append(n);
        }
        pos++;
    } catch (...) {
        delete curr;
        throw;
    }
    level--;
    debugMsg(QString("END"));
    curr->setLength(pos - off);
    return curr;
}

BValueNode *BDecoder::parseInt()
{
    Uint32 off = pos;
    pos++;
    QString n;
    // look for e and add everything between i and e to n
    while (pos < (Uint32)data.size() && data[pos] != 'e') {
        n += data[pos];
        pos++;
    }

    // check if we aren't at the end of the data
    if (pos >= (Uint32)data.size()) {
        throw Error(i18n("Unexpected end of input"));
    }

    // try to decode the int
    bool ok = true;
    int val = 0;
    val = n.toInt(&ok);
    if (ok) {
        pos++;
        debugMsg(QStringLiteral("INT = %1").arg(val));
        BValueNode *vn = new BValueNode(Value(val), off);
        vn->setLength(pos - off);
        return vn;
    } else {
        Int64 bi = 0LL;
        bi = n.toLongLong(&ok);
        if (!ok)
            throw Error(i18n("Cannot convert %1 to an int", n));

        pos++;
        debugMsg(QStringLiteral("INT64 = %1").arg(n));
        BValueNode *vn = new BValueNode(Value(bi), off);
        vn->setLength(pos - off);
        return vn;
    }
}

BValueNode *BDecoder::parseString()
{
    const Uint32 off = pos;
    // string are encoded 4:spam (length:string)

    // first get length by looking for the :
    while (pos < (Uint32)data.size() && data[pos] != ':') {
        pos++;
    }
    // check if we aren't at the end of the data
    if (pos >= (Uint32)data.size()) {
        throw Error(i18n("Unexpected end of input"));
    }

    // try to decode length
    bool ok = true;
    int len = 0;
    // This is an optimized version of QByteArray::fromRawData(data.constData() + off, pos - off).toInt(&ok)
    const char *start = data.constData() + off;
    const char *end = start + pos - off;
    while (start < end) {
        int n = *start++ - '0';
        if (n < 0 || n > 9) {
            ok = false;
            break;
        }
        len = (len << 3) + (len << 1) + n;
    }

    if (!ok || len < 0) {
        throw Error(i18n("Cannot convert %1 to an int", QString::fromUtf8(data.constData() + off, pos - off)));
    }
    // move pos to the first part of the string
    pos++;
    if (pos + len > (Uint32)data.size())
        throw Error(i18n("Torrent is incomplete."));

    const QByteArray arr(data.constData() + pos, len);
    pos += len;
    // read the string into n

    // pos should be positioned right after the string
    BValueNode *vn = new BValueNode(Value(arr), off);
    vn->setLength(pos - off);
    if (verbose) {
        if (arr.size() < 200)
            debugMsg(QStringLiteral("STRING ") + QString::fromUtf8(arr));
        else
            debugMsg(QStringLiteral("STRING really long string"));
    }
    return vn;
}

void BDecoder::debugMsg(const QString &msg)
{
    if (!verbose)
        return;

    Log &log = Out(SYS_GEN | LOG_DEBUG);
    for (int i = 0; i < level; i++)
        log << "-";

    log << msg << endl;
}

}
