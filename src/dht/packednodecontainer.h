/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DHT_PACKEDNODECONTAINER_H
#define DHT_PACKEDNODECONTAINER_H

#include <QByteArray>
#include <QList>

namespace dht
{
/*!
 * \headerfile dht/packednodecontainer.h
 * \brief Stores both nodes and nodes6 parameters of some DHT messages.
 */
class PackedNodeContainer
{
public:
    PackedNodeContainer();
    virtual ~PackedNodeContainer();

    //! Add a single node to the nodes or nodes2 parameter depending on it's size
    void addNode(const QByteArray &a);

    //! Get the nodes parameter
    [[nodiscard]] const QByteArray &getNodes() const
    {
        return nodes;
    }

    //! Get the nodes6 parameter
    [[nodiscard]] const QByteArray &getNodes6() const
    {
        return nodes6;
    }

protected:
    QByteArray nodes;
    QByteArray nodes6;
};

}

#endif // DHT_PACKEDNODECONTAINER_H
