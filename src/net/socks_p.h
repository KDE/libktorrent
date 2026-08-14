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
enum class Version : bt::Uint8 {
    VERSION_4 = 0x04,
};

enum class Command : bt::Uint8 {
    CONNECT = 0x01,
    BIND = 0x02,
};

enum class Reply : bt::Uint8 {
    OK = 0x5a,
    FAILED = 0x5b,
    FAILED_2 = 0x5c,
    FAILED_3 = 0x5d,
};

struct ConnectRequest {
    Version version;
    Command cmd;
    bt::Uint16 port;
    bt::Uint8 ip[4];
    char user_id[100];

    [[nodiscard]] int size() const
    {
        return 8 + strlen(user_id) + 1;
    }
};

struct ConnectReply {
    bt::Uint8 null_byte;
    Reply reply;
    bt::Uint8 dummy[6];
};
} // namespace socks4

namespace socks5
{
enum class Version : bt::Uint8 {
    VERSION_5 = 0x05,
};

enum class AddressType : bt::Uint8 {
    ADDR_IPV4 = 0x01,
    ADDR_DOMAIN = 0x03,
    ADDR_IPV6 = 0x04,
};

enum class Command : bt::Uint8 {
    CONNECT = 0x01,
    BIND = 0x02,
    UDP_ASSOCIATE = 0x03,
};

enum class Reply : bt::Uint8 {
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

enum class AuthMethod : bt::Uint8 {
    NONE = 0x00,
    GSSAPI = 0x01,
    USERNAME_PASSWORD = 0x02,
    NO_ACCEPTABLE_METHOD = 0xFF,
};

struct AuthRequest {
    Version version;
    bt::Uint8 nmethods;
    AuthMethod methods[5];

    [[nodiscard]] int size() const
    {
        return 2 + nmethods;
    }
};

struct AuthReply {
    Version version;
    AuthMethod method;
};

struct ConnectRequest {
    Version version;
    Command cmd;
    bt::Uint8 reserved;
    AddressType address_type;
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
    Version version;
    Reply reply;
    bt::Uint8 reserved;
    AddressType address_type;
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
