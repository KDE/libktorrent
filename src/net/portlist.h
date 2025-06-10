/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef NETPORTLIST_H
#define NETPORTLIST_H

#include <QList>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace net
{
enum Protocol {
    TCP,
    UDP,
};

/*!
 * \brief A network port.
 */
struct KTORRENT_EXPORT Port {
    bt::Uint16 number;
    Protocol proto;
    bool forward;

    Port();
    Port(bt::Uint16 number, Protocol proto, bool forward);
    Port(const Port &p);

    bool operator==(const Port &p) const;
};

/*!
 * \brief Listener for the PortList.
 */
class KTORRENT_EXPORT PortListener
{
public:
    virtual ~PortListener()
    {
    }

    /*!
     * A port has been added.
     * \param port The port
     */
    virtual void portAdded(const Port &port) = 0;

    /*!
     * A port has been removed
     * \param port The port
     */
    virtual void portRemoved(const Port &port) = 0;
};

/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * \brief List of ports which are currently being used.
 */
class KTORRENT_EXPORT PortList : public QList<Port>
{
    PortListener *lst;

public:
    PortList();
    virtual ~PortList();

    /*!
     * When a port is in use, this function needs to be called.
     * \param number Port number
     * \param proto Protocol
     * \param forward Whether or not it needs to be forwarded
     */
    void addNewPort(bt::Uint16 number, Protocol proto, bool forward);

    /*!
     * Needs to be called when a port is not being using anymore.
     * \param number Port number
     * \param proto Protocol
     */
    void removePort(bt::Uint16 number, Protocol proto);

    /*!
     * Set the port listener.
     * \param pl Port listener
     */
    void setListener(PortListener *pl)
    {
        lst = pl;
    }
};

}

#ifdef Q_CC_MSVC
#include <QHash>
inline size_t qHash(const net::Port &port, size_t seed = 0) noexcept
{
    return qHash((uint)port.number, seed);
}
#endif

#endif
