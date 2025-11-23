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
 * \headerfile bcodec/bnode.h
 * \author Joris Guisson
 * \brief Base class for a node in a bencoded piece of data.
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
     * Constructs a BNode of Type \a type with the underlying data \a data.
     *
     * The \c setData() method allows the bencoded data to be set after the
     * node has been created. This is useful for when a node will be updated
     * whilst still parsing the data.
     */
    BNode(Type type, QByteArrayView data = QByteArrayView());
    virtual ~BNode();

    //! Get the type of node
    [[nodiscard]] Type getType() const
    {
        return type;
    }

    /*!
     * Sets the bencoded data for this node to \a data.
     */
    void setBytes(QByteArrayView data)
    {
        this->data = data;
    }

    //! Get the bencoded data for this node.
    [[nodiscard]] QByteArrayView getBytes() const
    {
        return data;
    }

    //! Print some debugging info
    virtual void printDebugInfo() = 0;

private:
    Type type;
    QByteArrayView data;
};

/*!
 * \headerfile bcodec/bnode.h
 * \author Joris Guisson
 * \brief Represents a value (string, byte array or int) in bencoded data.
 *
 * @todo Use QVariant
 */
class KTORRENT_EXPORT BValueNode : public BNode
{
    Value value;

public:
    BValueNode(const Value &v, QByteArrayView data);
    ~BValueNode() override;

    [[nodiscard]] const Value &data() const
    {
        return value;
    }
    void printDebugInfo() override;
};

/*!
 * \headerfile bcodec/bnode.h
 * \author Joris Guisson
 * \brief Represents a dictionary in bencoded data.
 */
class KTORRENT_EXPORT BDictNode : public BNode
{
    struct DictEntry {
        QByteArrayView key;
        std::unique_ptr<BNode> node;

        DictEntry(QByteArrayView key, std::unique_ptr<BNode> node)
            : key(key)
            , node(std::move(node))
        {
        }
    };
    std::vector<DictEntry> children;

public:
    BDictNode();
    ~BDictNode() override;

    Q_DISABLE_COPY(BDictNode)

    //! Get a list of keys
    [[nodiscard]] QList<QByteArray> keys() const;

    /*!
     * Insert a BNode in the dictionary.
     * \param key The key
     * \param node The node
     */
    void insert(QByteArrayView key, std::unique_ptr<BNode> node);

    /*!
     * Get a BNode.
     * \param key The key
     * \return The node or nullptr if there is no node with has key \a key
     */
    BNode *getData(QByteArrayView key);

    /*!
     * Get a BListNode.
     * \param key The key
     * \return The node or nullptr if there is no list node with has key \a key
     */
    BListNode *getList(QByteArrayView key);

    /*!
     * Get a BDictNode.
     * \param key The key
     * \return The node or nullptr if there is no dict node with has key \a key
     */
    BDictNode *getDict(QByteArrayView key);

    /*!
     * Get a BValueNode.
     * \param key The key
     * \return The node or nullptr if there is no value node with has key \a key
     */
    BValueNode *getValue(QByteArrayView key);

    //! Same as getValue, except directly returns an int, if something goes wrong, an error will be thrown
    int getInt(QByteArrayView key);

    //! Same as getValue, except directly returns a qint64, if something goes wrong, an error will be thrown
    qint64 getInt64(QByteArrayView key);

    //! Same as getValue, except directly returns a QString, if something goes wrong, an error will be thrown
    QString getString(QByteArrayView key);

    /*!
     * Same as getValue, except directly returns an QByteArrayView, if something goes wrong, an error will be thrown.
     */
    QByteArrayView getByteArrayView(QByteArrayView key);

    /*!
     * Same as getValue, except directly returns an QByteArray, if something goes wrong, an error will be thrown.
     *
     * You should only use this function if you need a deep copy of the data. If you just need a view, then call getByteArrayView().
     */
    QByteArray getByteArray(QByteArrayView key);

    void printDebugInfo() override;
};

/*!
 * \headerfile bcodec/bnode.h
 * \author Joris Guisson
 * \brief Represents a list in bencoded data.
 */
class KTORRENT_EXPORT BListNode : public BNode
{
    std::vector<std::unique_ptr<BNode>> children;

public:
    BListNode();
    ~BListNode() override;

    Q_DISABLE_COPY(BListNode)

    /*!
     * Append a node to the list.
     * \param node The node
     */
    void append(std::unique_ptr<BNode> node);

    void printDebugInfo() override;

    //! Get the number of nodes in the list.
    [[nodiscard]] Uint32 getNumChildren() const
    {
        return children.size();
    }

    /*!
     * Get a node from the list
     * \param idx The index
     * \return The node or nullptr if idx is out of bounds
     */
    BNode *getChild(Uint32 idx)
    {
        return children[idx].get();
    }

    /*!
     * Get a BListNode.
     * \param idx The index
     * \return The node or nullptr if the index is out of bounds or the element
     *  at postion \a idx isn't a BListNode.
     */
    BListNode *getList(Uint32 idx);

    /*!
     * Get a BDictNode.
     * \param idx The index
     * \return The node or nullptr if the index is out of bounds or the element
     *  at postion \a idx isn't a BDictNode.
     */
    BDictNode *getDict(Uint32 idx);

    /*!
     * Get a BValueNode.
     * \param idx The index
     * \return The node or nullptr if the index is out of bounds or the element
     *  at postion \a idx isn't a BValueNode.
     */
    BValueNode *getValue(Uint32 idx);

    //! Same as getValue, except directly returns an int, if something goes wrong, an error will be thrown
    int getInt(Uint32 idx);

    //! Same as getValue, except directly returns a qint64, if something goes wrong, an error will be thrown
    qint64 getInt64(Uint32 idx);

    //! Same as getValue, except directly returns a QString, if something goes wrong, an error will be thrown
    QString getString(Uint32 idx);

    /**
     * Same as getValue, except directly returns an QByteArrayView, if something goes wrong, an error will be thrown.
     */
    QByteArrayView getByteArrayView(Uint32 idx);

    /**
     * Same as getValue, except directly returns an QByteArray, if something goes wrong, an error will be thrown
     *
     * You should only use this function if you need a deep copy of the data. If you just need a view, then call getByteArrayView().
     */
    QByteArray getByteArray(Uint32 idx);
};
}

#endif
