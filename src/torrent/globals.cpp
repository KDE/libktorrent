/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "globals.h"

#include "server.h"
#include <dht/dht.h>
#include <net/portlist.h>
#include <net/reverseresolver.h>
#include <utp/utpserver.h>

namespace bt
{
Globals *Globals::inst = nullptr;

Globals::Globals()
{
    plist = new net::PortList();
    tcp_server = nullptr;
    utp_server = nullptr;
    dh_table = new dht::DHT();
}

Globals::~Globals()
{
    // shutdown the reverse resolver thread
    net::ReverseResolver::shutdown();
    shutdownUTPServer();
    delete tcp_server;
    delete dh_table;
    delete plist;
}

Globals &Globals::instance()
{
    if (!inst)
        inst = new Globals();
    return *inst;
}

void Globals::cleanup()
{
    delete inst;
    inst = nullptr;
}

bool Globals::initTCPServer(Uint16 port)
{
    if (tcp_server)
        shutdownTCPServer();

    tcp_server = new Server();
    if (!tcp_server->changePort(port))
        return false;

    return true;
}

void Globals::shutdownTCPServer()
{
    delete tcp_server;
    tcp_server = nullptr;
}

bool Globals::initUTPServer(Uint16 port)
{
    if (utp_server)
        shutdownUTPServer();

    utp_server = new utp::UTPServer();
    if (!utp_server->changePort(port))
        return false;

    utp_server->start();
    return true;
}

void Globals::shutdownUTPServer()
{
    if (utp_server) {
        utp_server->stop();
        delete utp_server;
        utp_server = nullptr;
    }
}

}
