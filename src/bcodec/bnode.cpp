/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "bnode.h"
#include <QTextCodec>
#include <util/error.h>
#include <util/log.h>

namespace bt
{
BNode::BNode(Type type, Uint32 off)
    : type(type)
    , off(off)
    , len(0)
{
}

BNode::~BNode()
{
}

////////////////////////////////////////////////

BValueNode::BValueNode(const Value &v, Uint32 off)
    : BNode(VALUE, off)
    , value(v)
{
}

BValueNode::~BValueNode()
{
}

void BValueNode::printDebugInfo()
{
    if (value.getType() == Value::STRING)
        Out(SYS_GEN | LOG_DEBUG) << "Value = " << value.toString() << endl;
    else if (value.getType() == Value::INT)
        Out(SYS_GEN | LOG_DEBUG) << "Value = " << value.toInt() << endl;
    else if (value.getType() == Value::INT64)
        Out(SYS_GEN | LOG_DEBUG) << "Value = " << value.toInt64() << endl;
}

////////////////////////////////////////////////

BDictNode::BDictNode(Uint32 off)
    : BNode(DICT, off)
{
}

BDictNode::~BDictNode()
{
    QList<DictEntry>::iterator i = children.begin();
    while (i != children.end()) {
        DictEntry &e = *i;
        delete e.node;
        ++i;
    }
}

QList<QByteArray> BDictNode::keys() const
{
    QList<QByteArray> ret;
    ret.reserve(children.size());
    QList<DictEntry>::const_iterator i = children.begin();
    while (i != children.end()) {
        const DictEntry &e = *i;
        ret << e.key;
        ++i;
    }

    return ret;
}

void BDictNode::insert(const QByteArray &key, BNode *node)
{
    DictEntry entry;
    entry.key = key;
    entry.node = node;
    children.append(entry);
}

BNode *BDictNode::getData(const QByteArray &key)
{
    auto i = children.constBegin();
    while (i != children.constEnd()) {
        const DictEntry &e = *i;
        if (e.key == key)
            return e.node;
        i++;
    }
    return nullptr;
}

BDictNode *BDictNode::getDict(const QByteArray &key)
{
    QList<DictEntry>::iterator i = children.begin();
    while (i != children.end()) {
        DictEntry &e = *i;
        if (e.key == key)
            return dynamic_cast<BDictNode *>(e.node);
        ++i;
    }
    return nullptr;
}

BListNode *BDictNode::getList(const QByteArray &key)
{
    BNode *n = getData(key);
    return dynamic_cast<BListNode *>(n);
}

BValueNode *BDictNode::getValue(const QByteArray &key)
{
    BNode *n = getData(key);
    return dynamic_cast<BValueNode *>(n);
}

int BDictNode::getInt(const QByteArray &key)
{
    BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt();
}

qint64 BDictNode::getInt64(const QByteArray &key)
{
    BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT64 && v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt64();
}

QString BDictNode::getString(const QByteArray &key, QTextCodec *tc)
{
    BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    if (!tc)
        return v->data().toString();
    else
        return v->data().toString(tc);
}

QByteArray BDictNode::getByteArray(const QByteArray &key)
{
    BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toByteArray();
}

void BDictNode::printDebugInfo()
{
    Out(SYS_GEN | LOG_DEBUG) << "DICT" << endl;
    QList<DictEntry>::iterator i = children.begin();
    while (i != children.end()) {
        DictEntry &e = *i;
        Out(SYS_GEN | LOG_DEBUG) << QString::fromLatin1(e.key) << ": " << endl;
        e.node->printDebugInfo();
        ++i;
    }
    Out(SYS_GEN | LOG_DEBUG) << "END" << endl;
}

////////////////////////////////////////////////

BListNode::BListNode(Uint32 off)
    : BNode(LIST, off)
{
}

BListNode::~BListNode()
{
    for (int i = 0; i < children.count(); i++) {
        BNode *n = children.at(i);
        delete n;
    }
}

void BListNode::append(BNode *node)
{
    children.append(node);
}

BListNode *BListNode::getList(Uint32 idx)
{
    return dynamic_cast<BListNode *>(getChild(idx));
}

BDictNode *BListNode::getDict(Uint32 idx)
{
    return dynamic_cast<BDictNode *>(getChild(idx));
}

BValueNode *BListNode::getValue(Uint32 idx)
{
    return dynamic_cast<BValueNode *>(getChild(idx));
}

int BListNode::getInt(Uint32 idx)
{
    BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt();
}

qint64 BListNode::getInt64(Uint32 idx)
{
    BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT64 && v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt64();
}

QString BListNode::getString(Uint32 idx, QTextCodec *tc)
{
    BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    if (!tc)
        return v->data().toString();
    else
        return v->data().toString(tc);
}

QByteArray BListNode::getByteArray(Uint32 idx)
{
    BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toByteArray();
}

void BListNode::printDebugInfo()
{
    Out(SYS_GEN | LOG_DEBUG) << "LIST " << children.count() << endl;
    for (int i = 0; i < children.count(); i++) {
        BNode *n = children.at(i);
        n->printDebugInfo();
    }
    Out(SYS_GEN | LOG_DEBUG) << "END" << endl;
}
}
