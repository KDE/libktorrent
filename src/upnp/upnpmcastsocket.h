/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTUPNPMCASTSOCKET_H
#define KTUPNPMCASTSOCKET_H

#include <QSet>
#include <QUdpSocket>

#include "upnprouter.h"
#include <ktorrent_export.h>
#include <util/constants.h>
#include <util/ptrmap.h>

#include <memory>

namespace bt
{
class UPnPRouter;

/*!
 * \author Joris Guisson
 *
 * \brief Socket used to discover and keep track of UPnP devices.
 */
class KTORRENT_EXPORT UPnPMCastSocket : public QUdpSocket
{
    Q_OBJECT
public:
    UPnPMCastSocket(bool verbose = false);
    ~UPnPMCastSocket() override;

    //! Get the number of routers discovered
    Uint32 getNumDevicesDiscovered() const;

    //! Find a router using it's server name
    UPnPRouter *findDevice(const QString &name);

    //! Save all routers to a file (for convenience at startup)
    void saveRouters(const QString &file);

    //! Load all routers from a file
    void loadRouters(const QString &file);

    //! Set verbose mode
    void setVerbose(bool v);

public:
    /*!
     * Try to discover a UPnP device on the network.
     * A signal will be emitted when a device is found.
     */
    void discover();

private:
    void onReadyRead();
    void error(QAbstractSocket::SocketError err);
    void onXmlFileDownloaded(UPnPRouter *r, bool success);

Q_SIGNALS:
    /*!
     * Emitted when a router or internet gateway device is detected.
     * \param router The router
     */
    void discovered(bt::UPnPRouter *router);

private:
    class UPnPMCastSocketPrivate;
    std::unique_ptr<UPnPMCastSocketPrivate> d;
};
}

#endif
