/*
    SPDX-FileCopyrightText: 2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "socks.h"
#include "socks_p.h"
#include <sys/types.h>

#include <mse/encryptedpacketsocket.h>
#include <util/constants.h>
#include <util/log.h>

using namespace bt;

namespace net
{
bool Socks::socks_enabled = false;
int Socks::socks_version = 4;
QString Socks::socks_server_host;
bt::Uint16 Socks::socks_server_port = 1080;
QString Socks::socks_username;
QString Socks::socks_password;
bool Socks::socks_server_addr_resolved = false;
net::Address Socks::socks_server_addr;

Socks::Socks(mse::EncryptedPacketSocket *sock, const Address &dest)
    : sock(sock)
    , dest(dest)
    , state(IDLE)
    , internal_state(NONE)
{
    version = socks_version; // copy version in case it changes
}

Socks::~Socks()
{
}

void Socks::setSocksAuthentication(const QString &username, const QString &password)
{
    socks_username = username;
    socks_password = password;
}

void Socks::setSocksServerAddress(const QString &host, bt::Uint16 port)
{
    socks_server_addr_resolved = false;
    socks_server_host = host;
    socks_server_port = port;
}

void Socks::resolved(net::AddressResolver *ar)
{
    if (!ar->succeeded()) {
        state = FAILED;
        return;
    }

    socks_server_addr = ar->address();
    socks_server_addr_resolved = true;
    if (state == CONNECTING_TO_SERVER) {
        setup();
    }
}

Socks::State Socks::setup()
{
    state = CONNECTING_TO_SERVER;
    if (!socks_server_addr_resolved) {
        // resolve the address
        net::AddressResolver::resolve(socks_server_host, socks_server_port, this, SLOT(resolved(net::AddressResolver *)));
        return state;
    } else if (sock->connectTo(socks_server_addr)) {
        state = CONNECTING_TO_HOST;
        sock->setRemoteAddress(dest);
        return sendAuthRequest();
    } else if (sock->connecting()) {
        return state;
    } else {
        state = FAILED;
        return state;
    }
}

Socks::State Socks::onReadyToWrite()
{
    if (sock->connectSuccesFull()) {
        state = CONNECTING_TO_HOST;
        sock->setRemoteAddress(dest);
        return sendAuthRequest();
    } else {
        state = FAILED;
    }
    return state;
}

Socks::State Socks::onReadyToRead()
{
    if (state == CONNECTED) {
        return state;
    }

    if (sock->bytesAvailable() == 0) {
        state = FAILED;
        return state;
    }

    switch (internal_state) {
    case AUTH_REQUEST_SENT:
        return handleAuthReply();
        break;
    case USERNAME_AND_PASSWORD_SENT:
        return handleUsernamePasswordReply();
        break;
    case CONNECT_REQUEST_SENT:
        return handleConnectReply();
        break;
    default:
        return state;
    }
}

Socks::State Socks::sendAuthRequest()
{
    if (version == 5) {
        socks5::AuthRequest req;
        memset(&req, 0, sizeof(socks5::AuthRequest));
        req.version = socks5::Version::VERSION_5;
        if (socks_username.length() > 0 && socks_password.length() > 0) {
            req.nmethods = 2;
        } else {
            req.nmethods = 1;
        }
        req.methods[0] = socks5::AuthMethod::NONE; // No authentication
        req.methods[1] = socks5::AuthMethod::USERNAME_PASSWORD; // Username and password
        req.methods[2] = socks5::AuthMethod::GSSAPI; // GSSAPI
        sock->sendData((const Uint8 *)&req, req.size());
        internal_state = AUTH_REQUEST_SENT;
    } else {
        if (dest.protocol() == QAbstractSocket::IPv6Protocol) {
            Out(SYS_CON | LOG_IMPORTANT) << "SOCKSV4 does not support IPv6" << endl;
            state = FAILED;
            return state;
        }

        // version 4 has no auth request
        socks4::ConnectRequest req;
        memset(&req, 0, sizeof(socks4::ConnectRequest));
        req.version = socks4::Version::VERSION_4;
        req.cmd = socks4::Command::CONNECT;
        req.port = htons(dest.port());
        quint32 ip = htonl(dest.toIPv4Address());
        memcpy(req.ip, &ip, 4);
        strcpy(req.user_id, "KTorrent");
        sock->sendData((const Uint8 *)&req, req.size());
        internal_state = CONNECT_REQUEST_SENT;
        // Out(SYS_CON|LOG_DEBUG) << "SOCKSV4 send connect" << endl;
    }
    return state;
}

Socks::State Socks::handleAuthReply()
{
    socks5::AuthReply reply;
    if (sock->readData((Uint8 *)&reply, sizeof(socks5::AuthReply)) != sizeof(socks5::AuthReply)) {
        // Out(SYS_CON|LOG_DEBUG) << "sock->readData socks5::AuthReply size not ok" << endl;
        state = FAILED;
        return state;
    }

    if (reply.version != socks5::Version::VERSION_5 || reply.method == 0xFF) {
        // Out(SYS_CON|LOG_DEBUG) << "socks5::AuthReply = " << reply.version << " " << reply.method << endl;
        state = FAILED;
        return state;
    }

    switch (reply.method) {
    case socks5::AuthMethod::NONE:
        sendConnectRequest();
        break;
    case socks5::AuthMethod::USERNAME_PASSWORD:
        sendUsernamePassword();
        break;
    case socks5::AuthMethod::GSSAPI:
        break;
    }
    return state;
}

void Socks::sendUsernamePassword()
{
    //  Out(SYS_CON|LOG_DEBUG) << "Socks: sending username and password " << endl;
    QByteArray user = socks_username.toLocal8Bit();
    QByteArray pwd = socks_password.toLocal8Bit();
    Uint32 off = 0;
    Uint8 buffer[3 + 2 * 256];
    buffer[off++] = 0x01; // version
    buffer[off++] = user.size();
    memcpy(buffer + off, user.constData(), user.size());
    off += user.size();
    buffer[off++] = pwd.size();
    memcpy(buffer + off, pwd.constData(), pwd.size());
    off += pwd.size();
    sock->sendData(buffer, off);
    internal_state = USERNAME_AND_PASSWORD_SENT;
}

Socks::State Socks::handleUsernamePasswordReply()
{
    //  Out(SYS_CON|LOG_DEBUG) << "Socks: handleUsernamePasswordReply " << endl;
    Uint8 reply[2];
    if (sock->readData(reply, 2) != 2) {
        // Out(SYS_CON|LOG_DEBUG) << "sock->readData UPWReply size not ok" << endl;
        state = FAILED;
        return state;
    }

    if (reply[0] != 1 || reply[1] != 0) {
        Out(SYS_CON | LOG_IMPORTANT) << "Socks: Wrong username or password !" << endl;
        state = FAILED;
        return state;
    }

    sendConnectRequest();
    return state;
}

void Socks::sendConnectRequest()
{
    int len = 6;
    socks5::ConnectRequest req;
    memset(&req, 0, sizeof(socks5::ConnectRequest));
    req.version = socks5::Version::VERSION_5;
    req.cmd = socks5::Command::CONNECT;
    if (dest.protocol() == QAbstractSocket::IPv4Protocol) {
        quint32 ip = htonl(dest.toIPv4Address());
        memcpy(req.ipv4.ip, &ip, 4);
        req.ipv4.port = htons(dest.port());
        len += 4;
        req.address_type = socks5::AddressType::ADDR_IPV4;
    } else {
        Q_IPV6ADDR ip = dest.toIPv6Address();
        memcpy(req.ipv6.ip, ip.c, 16);
        req.ipv6.port = htons(dest.port());
        len += 16;
        req.address_type = socks5::AddressType::ADDR_IPV6;
    }
    sock->sendData((const Uint8 *)&req, len);
    internal_state = CONNECT_REQUEST_SENT;
}

Socks::State Socks::handleConnectReply()
{
    if (version == 4) {
        // Out(SYS_CON|LOG_DEBUG) << "SOCKSV4 handleConnectReply" << endl;
        socks4::ConnectReply reply;
        if (sock->readData((Uint8 *)&reply, sizeof(socks4::ConnectReply)) != sizeof(socks4::ConnectReply)) {
            //  Out(SYS_CON|LOG_DEBUG) << "sock->readData socks4::ConnectReply size not ok" << endl;
            state = FAILED;
            return state;
        }

        if (reply.reply != socks4::Reply::OK) {
            //  Out(SYS_CON|LOG_DEBUG) << "reply.reply != socks4::Reply::OK" << endl;
            state = FAILED;
            return state;
        }

        // Out(SYS_CON|LOG_DEBUG) << "SocksV4: connect OK ! " << endl;
        state = CONNECTED;
        return state;
    }

    socks5::ConnectReply reply;
    if (sock->readData((Uint8 *)&reply, sizeof(socks5::ConnectReply)) != sizeof(socks5::ConnectReply)) {
        // Out(SYS_CON|LOG_DEBUG) << "sock->readData socks5::ConnectReply size not ok" << endl;
        state = FAILED;
        return state;
    }

    if (reply.version != socks5::Version::VERSION_5 || reply.reply != socks5::Reply::OK) {
        // Out(SYS_CON|LOG_DEBUG) << "socks5::ConnectReply : " << reply.version << " " << reply.reply << " " << reply.address_type << endl;
        state = FAILED;
        return state;
    }

    Uint32 ba = sock->bytesAvailable();
    if (reply.address_type == socks5::AddressType::ADDR_IPV4) {
        Uint8 addr[6]; // IP and port
        if (ba < 6 || sock->readData(addr, 6) != 6) {
            // Out(SYS_CON|LOG_DEBUG) << "Failed to read IPv4 address : " << endl;
            state = FAILED;
            return state;
        } else {
            // Out(SYS_CON|LOG_DEBUG) << "Socks: connect OK ! " << endl;
            state = CONNECTED;
            return state;
        }
    } else if (reply.address_type == socks5::AddressType::ADDR_IPV6) {
        Uint8 addr[18]; // IP and port
        if (ba < 18 || sock->readData(addr, 6) != 6) {
            // Out(SYS_CON|LOG_DEBUG) << "Failed to read IPv4 address : " << endl;
            state = FAILED;
            return state;
        } else {
            // Out(SYS_CON|LOG_DEBUG) << "Socks: connect OK ! " << endl;
            state = CONNECTED;
            return state;
        }
    } else if (reply.address_type == socks5::AddressType::ADDR_DOMAIN) {
        Uint8 len = 0;
        if (ba == 0 || sock->readData(&len, 1) != 1) {
            // Out(SYS_CON|LOG_DEBUG) << "Failed to read domain name length " << endl;
            state = FAILED;
            return state;
        }
        ba = sock->bytesAvailable();
        Uint8 tmp[256];
        if (ba < len || sock->readData(tmp, len) != len) {
            // Out(SYS_CON|LOG_DEBUG) << "Failed to read domain name" << endl;
            state = FAILED;
            return state;
        } else {
            // Out(SYS_CON|LOG_DEBUG) << "Socks: connect OK ! " << endl;
            state = CONNECTED;
            return state;
        }
    } else {
        // Out(SYS_CON|LOG_DEBUG) << "Invalid address type : " << reply.address_type << endl;
        state = FAILED;
        return state;
    }
}
}

#include "moc_socks.cpp"
