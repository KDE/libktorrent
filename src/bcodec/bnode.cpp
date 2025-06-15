/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "bnode.h"
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
}

QList<QByteArray> BDictNode::keys() const
{
    QList<QByteArray> ret;
    ret.reserve(children.size());
    for (const DictEntry &e : children) {
        ret.push_back(e.key);
    }

    return ret;
}

void BDictNode::insert(const QByteArray &key, std::unique_ptr<BNode> node)
{
    children.emplace_back(key, std::move(node));
}

BNode *BDictNode::getData(const QByteArray &key)
{
    const auto i = std::find_if(children.cbegin(), children.cend(), [&key](const DictEntry &e) {
        return e.key == key;
    });
    return i == children.cend() ? nullptr : i->node.get();
}

BDictNode *BDictNode::getDict(const QByteArray &key)
{
    return dynamic_cast<BDictNode *>(getData(key));
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
    const BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt();
}

qint64 BDictNode::getInt64(const QByteArray &key)
{
    const BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT64 && v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt64();
}

QString BDictNode::getString(const QByteArray &key)
{
    const BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toString();
}

QByteArray BDictNode::getByteArray(const QByteArray &key)
{
    const BValueNode *v = getValue(key);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toByteArray();
}

void BDictNode::printDebugInfo()
{
    Out(SYS_GEN | LOG_DEBUG) << "DICT" << endl;
    for (const DictEntry &e : children) {
        Out(SYS_GEN | LOG_DEBUG) << QString::fromLatin1(e.key) << ": " << endl;
        e.node->printDebugInfo();
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
}

void BListNode::append(std::unique_ptr<BNode> node)
{
    children.push_back(std::move(node));
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
    const BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt();
}

qint64 BListNode::getInt64(Uint32 idx)
{
    const BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::INT64 && v->data().getType() != bt::Value::INT)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toInt64();
}

QString BListNode::getString(Uint32 idx)
{
    const BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toString();
}

QByteArray BListNode::getByteArray(Uint32 idx)
{
    const BValueNode *v = getValue(idx);
    if (!v)
        throw bt::Error(QStringLiteral("Key not found in dict"));

    if (v->data().getType() != bt::Value::STRING)
        throw bt::Error(QStringLiteral("Incompatible type"));

    return v->data().toByteArray();
}

void BListNode::printDebugInfo()
{
    Out(SYS_GEN | LOG_DEBUG) << "LIST " << children.size() << endl;
    for (const auto &n : children) {
        n->printDebugInfo();
    }
    Out(SYS_GEN | LOG_DEBUG) << "END" << endl;
}
}
