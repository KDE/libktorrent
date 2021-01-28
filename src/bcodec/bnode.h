/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#ifndef BTBNODE_H
#define BTBNODE_H

#include <QList>
#include <QVariant>
#include <QByteArray>
#include <util/constants.h>
#include <ktorrent_export.h>
#include "value.h"


namespace bt
{
class BListNode;

/**
 * @author Joris Guisson
 * @brief Base class for a node in a b-encoded piece of data
 *
 * There are 3 possible pieces of data in b-encoded piece of data.
 * This is the base class for all those 3 things.
*/
class KTORRENT_EXPORT BNode
{
public:
    enum Type {
        VALUE,
        DICT,
        LIST,
    };

    /**
     * Constructor, sets the Type, and the offset into
     * the data.
     * @param type Type of node
     * @param off The offset into the data
     */
    BNode(Type type, Uint32 off);
    virtual ~BNode();

    /// Get the type of node
    Type getType() const
    {
        return type;
    }

    /// Get the offset in the bytearray where this node starts.
    Uint32 getOffset() const
    {
        return off;
    }

    /// Get the length this node takes up in the bytearray.
    Uint32 getLength() const
    {
        return len;
    }

    /// Set the length
    void setLength(Uint32 l)
    {
        len = l;
    }

    /// Print some debugging info
    virtual void printDebugInfo() = 0;
private:
    Type type;
    Uint32 off, len;
};

/**
 * @author Joris Guisson
 * @brief Represents a value (string,bytearray or int) in bencoded data
 *
 * @todo Use QVariant
 */
class KTORRENT_EXPORT BValueNode : public BNode
{
    Value value;
public:
    BValueNode(const Value & v, Uint32 off);
    ~BValueNode() override;

    const Value & data() const
    {
        return value;
    }
    void printDebugInfo() override;
};

/**
 * @author Joris Guisson
 * @brief Represents a dictionary in bencoded data
 *
 */
class KTORRENT_EXPORT BDictNode : public BNode
{
    struct DictEntry {
        QByteArray key;
        BNode* node;
    };
    QList<DictEntry> children;
public:
    BDictNode(Uint32 off);
    ~BDictNode() override;

    /// Get a list of keys
    QList<QByteArray> keys() const;

    /**
     * Insert a BNode in the dictionary.
     * @param key The key
     * @param node The node
     */
    void insert(const QByteArray & key, BNode* node);

    /**
     * Get a BNode.
     * @param key The key
     * @return The node or 0 if there is no node with has key @a key
     */
    BNode* getData(const QByteArray & key);

    /**
     * Get a BListNode.
     * @param key The key
     * @return The node or 0 if there is no list node with has key @a key
     */
    BListNode* getList(const QByteArray& key);

    /**
     * Get a BDictNode.
     * @param key The key
     * @return The node or 0 if there is no dict node with has key @a key
     */
    BDictNode* getDict(const QByteArray& key);

    /**
     * Get a BValueNode.
     * @param key The key
     * @return The node or 0 if there is no value node with has key @a key
     */
    BValueNode* getValue(const QByteArray& key);

    /// Same as getValue, except directly returns an int, if something goes wrong, an error will be thrown
    int getInt(const QByteArray& key);

    /// Same as getValue, except directly returns a qint64, if something goes wrong, an error will be thrown
    qint64 getInt64(const QByteArray& key);

    /// Same as getValue, except directly returns a QString, if something goes wrong, an error will be thrown
    QString getString(const QByteArray& key, QTextCodec* tc);

    /// Same as getValue, except directly returns an QByteArray, if something goes wrong, an error will be thrown
    QByteArray getByteArray(const QByteArray& key);

    void printDebugInfo() override;
};

/**
 * @author Joris Guisson
 * @brief Represents a list in bencoded data
 *
 */
class KTORRENT_EXPORT BListNode : public BNode
{
    QList<BNode*> children;
public:
    BListNode(Uint32 off);
    ~BListNode() override;

    /**
     * Append a node to the list.
     * @param node The node
     */
    void append(BNode* node);
    void printDebugInfo() override;

    /// Get the number of nodes in the list.
    Uint32 getNumChildren() const
    {
        return children.count();
    }

    /**
     * Get a node from the list
     * @param idx The index
     * @return The node or 0 if idx is out of bounds
     */
    BNode* getChild(Uint32 idx)
    {
        return children.at(idx);
    }

    /**
     * Get a BListNode.
     * @param idx The index
     * @return The node or 0 if the index is out of bounds or the element
     *  at postion @a idx isn't a BListNode.
     */
    BListNode* getList(Uint32 idx);

    /**
     * Get a BDictNode.
     * @param idx The index
     * @return The node or 0 if the index is out of bounds or the element
     *  at postion @a idx isn't a BDictNode.
     */
    BDictNode* getDict(Uint32 idx);

    /**
     * Get a BValueNode.
     * @param idx The index
     * @return The node or 0 if the index is out of bounds or the element
     *  at postion @a idx isn't a BValueNode.
     */
    BValueNode* getValue(Uint32 idx);

    /// Same as getValue, except directly returns an int, if something goes wrong, an error will be thrown
    int getInt(Uint32 idx);

    /// Same as getValue, except directly returns a qint64, if something goes wrong, an error will be thrown
    qint64 getInt64(Uint32 idx);

    /// Same as getValue, except directly returns a QString, if something goes wrong, an error will be thrown
    QString getString(Uint32 idx, QTextCodec* tc);

    /// Same as getValue, except directly returns an QByteArray, if something goes wrong, an error will be thrown
    QByteArray getByteArray(Uint32 idx);
};
}

#endif
