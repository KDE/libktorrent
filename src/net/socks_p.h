/*
 *  SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
 *  SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NETSOCKS_P_H
#define NETSOCKS_P_H

#include <cstring>
#include <util/constants.h>

namespace net
{
namespace socks4
{
enum Version : bt::Uint8 {
    VERSION_4 = 0x04,
};

enum Command : bt::Uint8 {
    CONNECT = 0x01,
    BIND = 0x02,
};

enum Reply : bt::Uint8 {
    OK = 0x5a,
    FAILED = 0x5b,
    FAILED_2 = 0x5c,
    FAILED_3 = 0x5d,
};

struct ConnectRequest {
    bt::Uint8 version;
    bt::Uint8 cmd;
    bt::Uint16 port;
    bt::Uint8 ip[4];
    char user_id[100];

    int size() const
    {
        return 8 + strlen(user_id) + 1;
    }
};

struct ConnectReply {
    bt::Uint8 null_byte;
    bt::Uint8 reply;
    bt::Uint8 dummy[6];
};
} // namespace socks4

namespace socks5
{
enum Version : bt::Uint8 {
    VERSION_5 = 0x05,
};

enum AddressType : bt::Uint8 {
    IPV4 = 0x01,
    DOMAIN = 0x03,
    IPV6 = 0x04,
};

enum Command : bt::Uint8 {
    CONNECT = 0x01,
    BIND = 0x02,
    UDP_ASSOCIATE = 0x03,
};

enum Reply : bt::Uint8 {
    OK = 0x00, // succeeded
    SERVER_FAILURE = 0x01, // general SOCKS server failure
    NOT_ALLOWED = 0x02, // connection not allowed by ruleset
    NETWORK_UNREACHABLE = 0x03,
    HOST_UNREACHABLE = 0x04,
    CONNECTION_REFUSED = 0x05,
    TTL_EXPIRED = 0x06,
    CMD_NOT_SUPPORTED = 0x07,
    ADDR_TYPE_NOT_SUPPORTED = 0x08,
};

enum AuthMethod : bt::Uint8 {
    NONE = 0x00,
    GSSAPI = 0x01,
    USERNAME_PASSWORD = 0x02,
};

struct AuthRequest {
    bt::Uint8 version;
    bt::Uint8 nmethods;
    bt::Uint8 methods[5];

    int size() const
    {
        return 2 + nmethods;
    }
};

struct AuthReply {
    bt::Uint8 version;
    bt::Uint8 method;
};

struct ConnectRequest {
    bt::Uint8 version;
    bt::Uint8 cmd;
    bt::Uint8 reserved;
    bt::Uint8 address_type;
    union {
        struct {
            bt::Uint8 ip[4];
            bt::Uint16 port;
        } ipv4;
        struct {
            bt::Uint8 ip[16];
            bt::Uint16 port;
        } ipv6;
#if 0
        struct {
            bt::Uint8 len;
            char domain_name[200];
        } domain;
#endif
    };
};

struct ConnectReply {
    bt::Uint8 version;
    bt::Uint8 reply;
    bt::Uint8 reserved;
    bt::Uint8 address_type;
#if 0
    union {
        bt::Uint8 ip_v4[4];
        bt::Uint8 ip_v6[16];
    };
    bt::Uint16 port;
#endif
};
} // namespace socks5
} // namespace net

#endif // NETSOCKS_P_H
