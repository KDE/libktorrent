/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTGLOBALS_H
#define BTGLOBALS_H

#include <ktorrent_export.h>
#include <util/constants.h>

namespace utp
{
class UTPServer;
}

namespace net
{
class PortList;
}

namespace dht
{
class DHTBase;
}

namespace bt
{
class Server;

/*!
 * \brief Singleton object that manages other singletons, such as the TCP/UTP servers and DHT database.
 */
class KTORRENT_EXPORT Globals
{
public:
    virtual ~Globals();

    bool initTCPServer(Uint16 port);
    void shutdownTCPServer();

    bool initUTPServer(Uint16 port);
    void shutdownUTPServer();

    bool isUTPEnabled() const
    {
        return utp_server != nullptr;
    }
    bool isTCPEnabled() const
    {
        return tcp_server != nullptr;
    }

    Server &getTCPServer()
    {
        return *tcp_server;
    }
    dht::DHTBase &getDHT()
    {
        return *dh_table;
    }
    net::PortList &getPortList()
    {
        return *plist;
    }
    utp::UTPServer &getUTPServer()
    {
        return *utp_server;
    }

    static Globals &instance();
    static void cleanup();

private:
    Globals();

    Server *tcp_server;
    dht::DHTBase *dh_table;
    net::PortList *plist;
    utp::UTPServer *utp_server;

    static Globals *inst;
};
}

#endif
