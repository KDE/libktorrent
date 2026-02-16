/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "socket.h"
#include <QtGlobal>

#include <cerrno>
#include <cstring>
#include <sys/types.h>

#if defined(Q_OS_LINUX) && !defined(__FreeBSD_kernel__)
#include <asm/ioctls.h>
#endif

#ifdef Q_OS_SOLARIS
#include <sys/filio.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#include <fcntl.h>

#include <util/log.h>

#ifndef Q_OS_WIN
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <util/win32.h>
#include <ws2tcpip.h>
#define SHUT_RDWR SD_BOTH
#undef errno
#define errno WSAGetLastError()
#endif

using namespace bt;
using namespace Qt::Literals::StringLiterals;

namespace net
{
Socket::Socket(int fd, int ip_version)
    : SocketDevice(bt::TCP)
    , m_fd(fd)
    , m_ip_version(ip_version)
    , r_poll_index(-1)
    , w_poll_index(-1)
{
    // check if the IP version is 4 or 6
    if (m_ip_version != 4 && m_ip_version != 6) {
        m_ip_version = 4;
    }

    if (m_fd >= 0) {
        int val = 0;
#ifndef Q_OS_WIN
        socklen_t val_len = sizeof(val);
        int err = getsockopt(m_fd, SOL_SOCKET, SO_TYPE, &val, &val_len);
#else
        int val_len = sizeof(val);
        int err = getsockopt(m_fd, SOL_SOCKET, SO_TYPE, reinterpret_cast<char *>(&val), &val_len);
#endif
        if (err < 0) {
            err = errno;
            Out(SYS_GEN | LOG_IMPORTANT) << QStringLiteral("Failed to query socket type : %1").arg(QString::fromUtf8(strerror(err))) << endl;
        } else {
            transport_protocol = val == SOCK_STREAM ? bt::TCP : bt::UTP;
        }

        configureFd();
        cacheAddress();
    } else {
        m_state = CLOSED;
    }
}

Socket::Socket(bool tcp, int ip_version)
    : SocketDevice(tcp ? bt::TCP : bt::UTP)
    , m_fd(-1)
    , m_ip_version(ip_version)
    , r_poll_index(-1)
    , w_poll_index(-1)
{
    // check if the IP version is 4 or 6
    if (m_ip_version != 4 && m_ip_version != 6) {
        m_ip_version = 4;
    }

    m_state = CLOSED;

    reset();
}

Socket::~Socket()
{
    close();
}

void Socket::reset()
{
    close();

    int fd = socket(m_ip_version == 4 ? PF_INET : PF_INET6, transport_protocol == bt::TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (fd < 0) {
        const int err = errno;
        Out(SYS_GEN | LOG_IMPORTANT) << QStringLiteral("Cannot create socket : %1").arg(QString::fromUtf8(strerror(err))) << endl;
        return;
    }
    m_fd = fd;

    configureFd();

    m_state = IDLE;
}

void Socket::configureFd()
{
    // Try using dual-stack for IPv6
    dualstack = false;
    if (m_ip_version == 6) {
        const int no = 0;
#ifndef Q_OS_WIN
        int err = setsockopt(m_fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
#else
        int err = setsockopt(m_fd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char *>(&no), sizeof(no));
#endif
        if (err < 0) {
            err = errno;
            Out(SYS_GEN | LOG_IMPORTANT) << QStringLiteral("Failed to set dual-stack option for IPv6 socket : %1").arg(QString::fromUtf8(strerror(err)))
                                         << endl;
        } else {
            dualstack = true;
        }
    }

#if defined(Q_OS_MACX) || defined(Q_OS_DARWIN)
    int val = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val)) < 0) {
        const int err = errno;
        Out(SYS_CON | LOG_NOTICE) << QStringLiteral("Failed to set the NOSIGPIPE option : %1").arg(QString::fromUtf8(strerror(err))) << endl;
    }
#endif
}

void Socket::close()
{
    // Note: Reading the fd into a local variable works around a possible codegen issue that results
    //       in valgrind warnings about close() being called on invalid fd of -1.
    //       https://invent.kde.org/network/libktorrent/-/merge_requests/80
    const int fd = m_fd;
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
#ifdef Q_OS_WIN
        ::closesocket(fd);
#else
        ::close(fd);
#endif
        m_fd = -1;
        m_state = CLOSED;
    }
}

void Socket::setBlocking(bool on)
{
#ifndef Q_OS_WIN
    int flag = fcntl(m_fd, F_GETFL, 0);
    if (!on) {
        fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
    } else {
        fcntl(m_fd, F_SETFL, flag & ~O_NONBLOCK);
    }
#else
    /** This makes no sense...

        > FIONBIO
        > -------
        > The lpvInBuffer parameter points at an unsigned long (QoS), which is
        > nonzero if non-blocking mode is to be enabled and zero if it is to be
        > disabled.

        https://learn.microsoft.com/en-us/windows/win32/winsock/winsock-ioctls#fionbio

        But here we have to pass a pointer to 0 to enable the non-blocking mode?
    */
    u_long b = on ? 0 : 1;
    ioctlsocket(m_fd, FIONBIO, &b);
#endif
}

bool Socket::connectTo(const Address &a)
{
    int len = 0;
    struct sockaddr_storage ss;
    a.toSocketAddress(&ss, len, dualstack);
    if (::connect(m_fd, (struct sockaddr *)&ss, len) < 0) {
        const int err = errno;
#ifndef Q_OS_WIN
        if (err == EINPROGRESS)
#else
        if (err == WSAEINVAL || err == WSAEALREADY || err == WSAEWOULDBLOCK)
#endif
        {
            //  Out(SYS_CON|LOG_DEBUG) << "Socket is connecting" << endl;
            m_state = CONNECTING;
            return false;
        } else {
            Out(SYS_CON | LOG_NOTICE) << QStringLiteral("Cannot connect to host %1 : %2").arg(a.toString(), QString::fromUtf8(strerror(err))) << endl;
            return false;
        }
    }
    m_state = CONNECTED;
    cacheAddress();
    return true;
}

bool Socket::bind(const QString &ip, Uint16 port, bool also_listen)
{
    return bind(net::Address(ip, port), also_listen);
}

bool Socket::bind(const net::Address &addr, bool also_listen)
{
    int val = 1;
#ifndef Q_OS_WIN
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0)
#else
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int)) < 0)
#endif
    {
        const int err = errno;
        Out(SYS_CON | LOG_NOTICE) << QStringLiteral("Failed to set the reuseaddr option : %1").arg(QString::fromUtf8(strerror(err))) << endl;
    }

    int len = 0;
    struct sockaddr_storage ss;
    addr.toSocketAddress(&ss, len);
    if (::bind(m_fd, (struct sockaddr *)&ss, len) != 0) {
        const int err = errno;
        Out(SYS_CON | LOG_IMPORTANT)
            << QStringLiteral("Cannot bind to port %1:%2 : %3").arg(addr.toString()).arg(addr.port()).arg(QString::fromUtf8(strerror(err))) << endl;
        return false;
    }

    if (also_listen && listen(m_fd, SOMAXCONN) < 0) {
        const int err = errno;
        Out(SYS_CON | LOG_IMPORTANT)
            << QStringLiteral("Cannot listen to port %1:%2 : %3").arg(addr.toString()).arg(addr.port()).arg(QString::fromUtf8(strerror(err))) << endl;
        return false;
    }

    m_state = BOUND;
    return true;
}

int Socket::send(const bt::Uint8 *buf, int len)
{
#ifndef Q_OS_WIN
    int ret = ::send(m_fd, buf, len, MSG_NOSIGNAL);
#else
    int ret = ::send(m_fd, (char *)buf, len, MSG_NOSIGNAL);
#endif
    if (ret < 0) {
        const int err = errno;
        if (err != EAGAIN && err != EWOULDBLOCK) {
            //  Out(SYS_CON|LOG_DEBUG) << "Send error : " << QString::fromUtf8(strerror(err)) << endl;
            close();
        }
        return 0;
    }
    return ret;
}

int Socket::recv(bt::Uint8 *buf, int max_len)
{
#ifndef Q_OS_WIN
    int ret = ::recv(m_fd, buf, max_len, 0);
#else
    int ret = ::recv(m_fd, (char *)buf, max_len, 0);
#endif
    if (ret < 0) {
        const int err = errno;
        if (err != EAGAIN && err != EWOULDBLOCK) {
            //  Out(SYS_CON|LOG_DEBUG) << "Receive error : " << QString::fromUtf8(strerror(err)) << endl;
            close();
            return 0;
        }

        return ret;
    } else if (ret == 0) {
        // connection closed
        close();
        return 0;
    }
    return ret;
}

int Socket::sendTo(const bt::Uint8 *buf, int len, const Address &a)
{
    int alen = 0;
    struct sockaddr_storage ss;
    a.toSocketAddress(&ss, alen, dualstack);
    int ret = ::sendto(m_fd, (char *)buf, len, 0, (struct sockaddr *)&ss, alen);
    if (ret < 0) {
        const int err = errno;
        if (err == EAGAIN || err == EWOULDBLOCK) {
            return SEND_WOULD_BLOCK;
        }

        Out(SYS_CON | LOG_DEBUG) << "Send error : " << QString::fromUtf8(strerror(err)) << endl;
        return SEND_FAILURE;
    }

    return ret;
}

int Socket::recvFrom(bt::Uint8 *buf, int max_len, Address &a)
{
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
#ifndef Q_OS_WIN
    int ret = ::recvfrom(m_fd, buf, max_len, 0, (struct sockaddr *)&ss, &slen);
#else
    int ret = ::recvfrom(m_fd, (char *)buf, max_len, 0, (struct sockaddr *)&ss, &slen);
#endif
    if (ret < 0) {
        const int err = errno;
        Out(SYS_CON | LOG_DEBUG) << "Receive error : " << QString::fromUtf8(strerror(err)) << endl;
        return 0;
    }
    a = ss;
    return ret;
}

int Socket::accept(Address &a)
{
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int sfd = ::accept(m_fd, (struct sockaddr *)&ss, &slen);

    if (sfd < 0) {
        const int err = errno;
        Out(SYS_CON | LOG_DEBUG) << "Accept error : " << QString::fromUtf8(strerror(err)) << endl;
        return -1;
    }
    a = net::Address(&ss);

    Out(SYS_CON | LOG_DEBUG) << "Accepted connection from " << a.toString() << endl;
    return sfd;
}

bool Socket::setTOS(unsigned char type_of_service)
{
    // If type of service is 0, do nothing
    if (type_of_service == 0) {
        return true;
    }

    if (m_ip_version == 4) {
#if defined(Q_OS_MACX) || defined(Q_OS_DARWIN) || defined(Q_OS_NETBSD) || defined(Q_OS_BSD4)
        unsigned int c = type_of_service;
#else
        unsigned char c = type_of_service;
#endif
#ifndef Q_OS_WIN
        if (setsockopt(m_fd, IPPROTO_IP, IP_TOS, &c, sizeof(c)) < 0)
#else
        if (setsockopt(m_fd, IPPROTO_IP, IP_TOS, (char *)&c, sizeof(c)) < 0)
#endif
        {
            const int err = errno;
            Out(SYS_CON | LOG_NOTICE) << QStringLiteral("Failed to set TOS to %1 : %2").arg((int)type_of_service).arg(QString::fromUtf8(strerror(err))) << endl;
            return false;
        }
    } else {
#if defined(IPV6_TCLASS)
#ifndef Q_OS_WIN
        int c = type_of_service;
#else
        char c = type_of_service;
#endif
        if (setsockopt(m_fd, IPPROTO_IPV6, IPV6_TCLASS, &c, sizeof(c)) < 0) {
            const int err = errno;
            Out(SYS_CON | LOG_NOTICE)
                << QStringLiteral("Failed to set traffic class to %1 : %2").arg((int)type_of_service).arg(QString::fromUtf8(strerror(err))) << endl;
            return false;
        }
#endif
    }

    return true;
}

Uint32 Socket::bytesAvailable() const
{
    int ret = 0;
#ifndef Q_OS_WIN
    if (ioctl(m_fd, FIONREAD, &ret) < 0)
#else
    if (ioctlsocket(m_fd, FIONREAD, (u_long *)&ret) < 0)
#endif
        return 0;

    return ret;
}

bool Socket::connectSuccesFull()
{
    if (m_state != CONNECTING && m_state != CONNECTED) {
        return false;
    }

    int err = 0;
    socklen_t len = sizeof(int);
#ifndef Q_OS_WIN
    if (getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
#else
    if (getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len) < 0)
#endif
        return false;

    if (err == 0) {
        m_state = CONNECTED;
        cacheAddress();
    }

    return err == 0;
}

void Socket::cacheAddress()
{
    struct sockaddr_storage ss; /* Where the peer adr goes. */
    socklen_t sslen = sizeof(ss);

    if (getpeername(m_fd, (struct sockaddr *)&ss, &sslen) == 0) {
        addr = net::Address(&ss);
    }
}

Address Socket::getSockName() const
{
    struct sockaddr_storage ss; /* Where the peer adr goes. */
    socklen_t sslen = sizeof(ss);

    if (getsockname(m_fd, (struct sockaddr *)&ss, &sslen) == 0) {
        return net::Address(&ss);
    } else {
        return Address();
    }
}

int Socket::take()
{
    int ret = m_fd;
    m_fd = -1;
    m_state = CLOSED;
    return ret;
}

void Socket::prepare(Poll *p, Poll::Mode mode)
{
    if (m_fd >= 0) {
        if (mode == Poll::OUTPUT) {
            w_poll_index = p->add(m_fd, mode);
        } else {
            r_poll_index = p->add(m_fd, mode);
        }
    }
}

bool Socket::ready(const Poll *p, Poll::Mode mode) const
{
    return p->ready(mode == Poll::OUTPUT ? w_poll_index : r_poll_index, mode);
}

}
