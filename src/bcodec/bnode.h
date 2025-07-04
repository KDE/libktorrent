/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTBNODE_H
#define BTBNODE_H

#include "value.h"
#include <QByteArray>
#include <QList>
#include <QVariant>
#include <ktorrent_export.h>
#include <util/constants.h>

#include <memory>
#include <vector>

namespace bt
{
class BListNode;

/*!
 * \author Joris Guisson
 * \brief Base class for a node in a b-encoded piece of data
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

    /*!
     * Constructor, sets the Type, and the offset into
     * the data.
     * \param type Type of node
     * \param off The offset into the data
     */
    BNode(Type type, Uint32 off);
    virtual ~BNode();

    //! Get the type of node
    Type getType() const
    {
        return type;
    }

    //! Get the offset in the bytearray where this node starts.
    Uint32 getOffset() const
    {
        return off;
    }

    //! Get the length this node takes up in the bytearray.
    Uint32 getLength() const
    {
        return len;
    }

    //! Set the length
    void setLength(Uint32 l)
    {
        len = l;
    }

    //! Print some debugging info
    virtual void printDebugInfo() = 0;

private:
    Type type;
    Uint32 off, len;
};

/*!
 * \author Joris Guisson
 * \brief Represents a value (string,bytearray or int) in bencoded data
 *
 * @todo Use QVariant
 */
class KTORRENT_EXPORT BValueNode : public BNode
{
    Value value;

public:
    BValueNode(const Value &v, Uint32 off);
    ~BValueNode() override;

    const Value &data() const
    {
        return value;
    }
    void printDebugInfo() override;
};

/*!
 * \author Joris Guisson
 * \brief Represents a dictionary in bencoded data
 *
 */
class KTORRENT_EXPORT BDictNode : public BNode
{
    struct DictEntry {
        QByteArray key;
        std::unique_ptr<BNode> node;

        DictEntry(const QByteArray &key, std::unique_ptr<BNode> node)
            : key(key)
            , node(std::move(node))
        {
        }
    };
    std::vector<DictEntry> children;

public:
    BDictNode(Uint32 off);
    ~BDictNode() override;

    Q_DISABLE_COPY(BDictNode)

    //! Get a list of keys
    QList<QByteArray> keys() const;

    /*!
     * Insert a BNode in the dictionary.
     * \param key The key
     * \param node The node
     */
    void insert(const QByteArray &key, std::unique_ptr<BNode> node);

    /*!
     * Get a BNode.
     * \param key The key
     * \return The node or 0 if there is no node with has key \a key
     */
    BNode *getData(const QByteArray &key);

    /*!
     * Get a BListNode.
     * \param key The key
     * \return The node or 0 if there is no list node with has key \a key
     */
    BListNode *getList(const QByteArray &key);

    /*!
     * Get a BDictNode.
     * \param key The key
     * \return The node or 0 if there is no dict node with has key \a key
     */
    BDictNode *getDict(const QByteArray &key);

    /*!
     * Get a BValueNode.
     * \param key The key
     * \return The node or 0 if there is no value node with has key \a key
     */
    BValueNode *getValue(const QByteArray &key);

    //! Same as getValue, except directly returns an int, if something goes wrong, an error will be thrown
    int getInt(const QByteArray &key);

    //! Same as getValue, except directly returns a qint64, if something goes wrong, an error will be thrown
    qint64 getInt64(const QByteArray &key);

    //! Same as getValue, except directly returns a QString, if something goes wrong, an error will be thrown
    QString getString(const QByteArray &key);

    //! Same as getValue, except directly returns an QByteArray, if something goes wrong, an error will be thrown
    QByteArray getByteArray(const QByteArray &key);

    void printDebugInfo() override;
};

/*!
 * \author Joris Guisson
 * \brief Represents a list in bencoded data
 *
 */
class KTORRENT_EXPORT BListNode : public BNode
{
    std::vector<std::unique_ptr<BNode>> children;

public:
    BListNode(Uint32 off);
    ~BListNode() override;

    Q_DISABLE_COPY(BListNode)

    /*!
     * Append a node to the list.
     * \param node The node
     */
    void append(std::unique_ptr<BNode> node);

    void printDebugInfo() override;

    //! Get the number of nodes in the list.
    Uint32 getNumChildren() const
    {
        return children.size();
    }

    /*!
     * Get a node from the list
     * \param idx The index
     * \return The node or 0 if idx is out of bounds
     */
    BNode *getChild(Uint32 idx)
    {
        return children[idx].get();
    }

    /*!
     * Get a BListNode.
     * \param idx The index
     * \return The node or 0 if the index is out of bounds or the element
     *  at postion \a idx isn't a BListNode.
     */
    BListNode *getList(Uint32 idx);

    /*!
     * Get a BDictNode.
     * \param idx The index
     * \return The node or 0 if the index is out of bounds or the element
     *  at postion \a idx isn't a BDictNode.
     */
    BDictNode *getDict(Uint32 idx);

    /*!
     * Get a BValueNode.
     * \param idx The index
     * \return The node or 0 if the index is out of bounds or the element
     *  at postion \a idx isn't a BValueNode.
     */
    BValueNode *getValue(Uint32 idx);

    //! Same as getValue, except directly returns an int, if something goes wrong, an error will be thrown
    int getInt(Uint32 idx);

    //! Same as getValue, except directly returns a qint64, if something goes wrong, an error will be thrown
    qint64 getInt64(Uint32 idx);

    //! Same as getValue, except directly returns a QString, if something goes wrong, an error will be thrown
    QString getString(Uint32 idx);

    //! Same as getValue, except directly returns an QByteArray, if something goes wrong, an error will be thrown
    QByteArray getByteArray(Uint32 idx);
};
}

#endif
