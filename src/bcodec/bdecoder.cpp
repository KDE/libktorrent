/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "bdecoder.h"
#include "bnode.h"
#include <KLocalizedString>
#include <util/error.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

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

std::unique_ptr<BNode> BDecoder::decode()
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

std::unique_ptr<BDictNode> BDecoder::decodeDict()
{
    std::unique_ptr<BNode> n = decode();
    if (n && n->getType() == BNode::DICT)
        return std::unique_ptr<BDictNode>(static_cast<BDictNode *>(n.release()));

    return nullptr;
}

std::unique_ptr<BListNode> BDecoder::decodeList()
{
    std::unique_ptr<BNode> n = decode();
    if (n && n->getType() == BNode::LIST)
        return std::unique_ptr<BListNode>(static_cast<BListNode *>(n.release()));

    return nullptr;
}

std::unique_ptr<BDictNode> BDecoder::parseDict()
{
    Uint32 off = pos;
    // we're now entering a dictionary
    auto curr = std::make_unique<BDictNode>(off);
    pos++;
    debugMsg(u"DICT"_s);
    level++;

    while (pos < (Uint32)data.size() && data[pos] != 'e') {
        debugMsg(u"Key : "_s);
        const std::unique_ptr<BNode> kn = decode();
        const BValueNode *k = dynamic_cast<BValueNode *>(kn.get());
        if (!k || k->data().getType() != Value::STRING) {
            throw Error(i18n("Decode error"));
        }

        QByteArray key = k->data().toByteArray();

        auto value = decode();
        if (!value)
            throw Error(i18n("Decode error"));

        curr->insert(key, std::move(value));
    }
    pos++;

    level--;
    debugMsg(u"END"_s);
    curr->setLength(pos - off);
    return curr;
}

std::unique_ptr<BListNode> BDecoder::parseList()
{
    Uint32 off = pos;
    debugMsg(u"LIST"_s);
    level++;
    auto curr = std::make_unique<BListNode>(off);
    pos++;

    while (pos < (Uint32)data.size() && data[pos] != 'e') {
        std::unique_ptr<BNode> n = decode();
        if (n)
            curr->append(std::move(n));
    }
    pos++;

    level--;
    debugMsg(u"END"_s);
    curr->setLength(pos - off);
    return curr;
}

std::unique_ptr<BValueNode> BDecoder::parseInt()
{
    Uint32 off = pos;
    pos++;
    QString n;
    // look for e and add everything between i and e to n
    while (pos < (Uint32)data.size() && data[pos] != 'e') {
        n += QLatin1Char(data[pos]);
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
        auto vn = std::make_unique<BValueNode>(Value(val), off);
        vn->setLength(pos - off);
        return vn;
    } else {
        Int64 bi = 0LL;
        bi = n.toLongLong(&ok);
        if (!ok)
            throw Error(i18n("Cannot convert %1 to an int", n));

        pos++;
        debugMsg(QStringLiteral("INT64 = %1").arg(n));
        auto vn = std::make_unique<BValueNode>(Value(bi), off);
        vn->setLength(pos - off);
        return vn;
    }
}

std::unique_ptr<BValueNode> BDecoder::parseString()
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
    auto vn = std::make_unique<BValueNode>(Value(arr), off);
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
