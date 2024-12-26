/*
    SPDX-FileCopyrightText: 2006 Dan Kennedy.
    SPDX-FileCopyrightText: 2006 Juliusz Chroboczek.

    SPDX-License-Identifier: MIT
*/

#include "win32.h"
#include <assert.h>
#include <cerrno>
#include <cstdlib>
#include <malloc.h>
// #undef poll
// #undef socket
// #undef connect
// #undef accept
// #undef shutdown
// #undef getpeername
// #undef sleep
// #undef stat

#if 0
unsigned int
mingw_sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}

int mingw_close_socket(SOCKET fd)
{
    int rc;

    rc = closesocket(fd);
    assert(rc == 0);
    return 0;
}

static void
set_errno(int winsock_err)
{
    switch (winsock_err) {
    case WSAEWOULDBLOCK:
        errno = EAGAIN;
        break;
    default:
        errno = winsock_err;
        break;
    }
}

int mingw_write_socket(SOCKET fd, void *buf, int n)
{
    int rc = send(fd, buf, n, 0);
    if (rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}

int mingw_read_socket(SOCKET fd, void *buf, int n)
{
    int rc = recv(fd, buf, n, 0);
    if (rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}


/*
* Set the "non-blocking" flag on socket fd to the value specified by
* the second argument (i.e. if the nonblocking argument is non-zero, the
* socket is set to non-blocking mode). Zero is returned if the operation
* is successful, other -1.
*/
int
mingw_setnonblocking(SOCKET fd, int nonblocking)
{
    int rc;

    unsigned long mode = 1;
    rc = ioctlsocket(fd, FIONBIO, &mode);
    if (rc != 0) {
        set_errno(WSAGetLastError());
    }
    return (rc == 0 ? 0 : -1);
}

/*
* A wrapper around the socket() function. The purpose of this wrapper
* is to ensure that the global errno symbol is set if an error occurs,
* even if we are using winsock.
*/
SOCKET
mingw_socket(int domain, int type, int protocol)
{
    SOCKET fd = socket(domain, type, protocol);
    if (fd == INVALID_SOCKET) {
        set_errno(WSAGetLastError());
    }
    return fd;
}

static void
set_connect_errno(int winsock_err)
{
    switch (winsock_err) {
    case WSAEINVAL:
    case WSAEALREADY:
    case WSAEWOULDBLOCK:
        errno = EINPROGRESS;
        break;
    default:
        errno = winsock_err;
        break;
    }
}

/*
* A wrapper around the connect() function. The purpose of this wrapper
* is to ensure that the global errno symbol is set if an error occurs,
* even if we are using winsock.
*/
int
mingw_connect(SOCKET fd, struct sockaddr *addr, socklen_t addr_len)
{
    int rc = connect(fd, addr, addr_len);
    assert(rc == 0 || rc == SOCKET_ERROR);
    if (rc == SOCKET_ERROR) {
        set_connect_errno(WSAGetLastError());
    }
    return rc;
}

/*
* A wrapper around the accept() function. The purpose of this wrapper
* is to ensure that the global errno symbol is set if an error occurs,
* even if we are using winsock.
*/
SOCKET
mingw_accept(SOCKET fd, struct sockaddr *addr, socklen_t *addr_len)
{
    SOCKET newfd = accept(fd, addr, addr_len);
    if (newfd == INVALID_SOCKET) {
        set_errno(WSAGetLastError());
        newfd = -1;
    }
    return newfd;
}

/*
* A wrapper around the shutdown() function. The purpose of this wrapper
* is to ensure that the global errno symbol is set if an error occurs,
* even if we are using winsock.
*/
int
mingw_shutdown(SOCKET fd, int mode)
{
    int rc = shutdown(fd, mode);
    assert(rc == 0 || rc == SOCKET_ERROR);
    if (rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}

/*
* A wrapper around the getpeername() function. The purpose of this wrapper
* is to ensure that the global errno symbol is set if an error occurs,
* even if we are using winsock.
*/
int
mingw_getpeername(SOCKET fd, struct sockaddr *name, socklen_t *namelen)
{
    int rc = getpeername(fd, name, namelen);
    assert(rc == 0 || rc == SOCKET_ERROR);
    if (rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}

/* Stat doesn't work on directories if the name ends in a slash. */

int
mingw_stat(const char *filename, struct stat *ss)
{
    int len, rc, saved_errno;
    char *noslash;

    len = strlen(filename);
    if (len <= 1 || filename[len - 1] != '/')
        return stat(filename, ss);

    noslash = malloc(len);
    if (noslash == NULL)
        return -1;

    memcpy(noslash, filename, len - 1);
    noslash[len - 1] = '\0';

    rc = stat(noslash, ss);
    saved_errno = errno;
    free(noslash);
    errno = saved_errno;
    return rc;
}
#endif
char *mingw_strerror(int error)
{
#ifdef UNICODE
    wchar_t message[1024];
#else
    char message[1024];
#endif
    static char cmessage[1024];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  error,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  message,
                  sizeof(message),
                  NULL);
#ifdef UNICODE
    wcstombs(cmessage, message, 1024);
#endif
    char *p;
    for (p = cmessage; *p; p++) {
        if (*p == '\n' || *p == '\r')
            *p = ' ';
    }

    return cmessage;
}

#if 0
static int init(struct pollfd *pollfds, nfds_t nfds, SOCKET *fds, HANDLE *hEvents)
{
    nfds_t i;

    for (i = 0; i < nfds; i++) {
        fds[i] = INVALID_SOCKET;
        hEvents[i] = NULL;
    }

    for (i = 0; i < nfds; i++) {
        fds[i] = pollfds[i].fd;
        hEvents[i] = WSACreateEvent();
        pollfds[i].revents = 0;

        if (WSAEventSelect(fds[i], hEvents[i], pollfds[i].events) < 0) {
            errno = WSAGetLastError();
            return -1;
        }
    }

    return 0;
}

static void clean(nfds_t nfds, SOCKET *fds, HANDLE *hEvents)
{
    nfds_t i;

    for (i = 0; i < nfds; i++) {
        if (fds[i] != INVALID_SOCKET) {
            WSAEventSelect(fds[i], NULL, 0);
        }

        if (hEvents[i] != NULL) {
            WSACloseEvent(hEvents[i]);
        }
    }
}

int poll(struct pollfd *pollfds, nfds_t nfds, int timeout)
{
    SOCKET *fds;
    HANDLE *hEvents;
    DWORD n;

    fds = (SOCKET *)alloca(sizeof(SOCKET) * nfds);
    hEvents = (HANDLE *)alloca(sizeof(HANDLE) * nfds);
    if (init(pollfds, nfds, fds, hEvents) < 0) {
        clean(nfds, fds, hEvents);
        return -1;
    }

    n = WSAWaitForMultipleEvents(nfds, hEvents, FALSE, timeout, FALSE);
    if (n == WSA_WAIT_FAILED) {
        clean(nfds, fds, hEvents);
        return -1;
    } else if (n == WSA_WAIT_TIMEOUT) {
        clean(nfds, fds, hEvents);
        return 0;
    } else {
        SOCKET fd;
        HANDLE hEvent;
        WSANETWORKEVENTS events;

        n -= WSA_WAIT_EVENT_0;
        fd = fds[n];
        hEvent = hEvents[n];

        if (WSAEnumNetworkEvents(fd, hEvent, &events) < 0) {
            clean(nfds, fds, hEvents);
            return -1;
        }

        pollfds[n].revents = (short) events.lNetworkEvents;
        clean(nfds, fds, hEvents);
        return n + 1;
    }
}
#endif
