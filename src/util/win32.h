/*
    SPDX-FileCopyrightText: 2006 Dan Kennedy.
    SPDX-FileCopyrightText: 2006 Juliusz Chroboczek.

    SPDX-License-Identifier: MIT
*/

/*
 * Polipo was originally designed to run on Unix-like systems. This
 * header file (and it's accompanying implementation file mingw.c) contain
 * code that allows polipo to run on Microsoft Windows too.
 *
 * The target MS windows compiler is Mingw (MINimal Gnu for Windows). The
 * code in this file probably get's us pretty close to MSVC also, but
 * this has not been tested. To build polipo for Mingw, define the MINGW
 * symbol. For Unix or Unix-like systems, leave it undefined.
 */
#ifndef MINGW_H
#define MINGW_H
/* Unfortunately, there's no hiding it. */
// #define HAVE_WINSOCK 1

/* At time of writing, a fair bit of stuff doesn't work under Mingw.
 * Hopefully they will be fixed later (especially the disk-cache).
 */

#include <io.h>
#include <ktorrent_export.h>
#include <wchar.h>
/* Pull in winsock.h for (almost) berkeley sockets. */
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE 1000
#include <winsock2.h>
// #define ENOTCONN        WSAENOTCONN
// #define EWOULDBLOCK     WSAEWOULDBLOCK
// #define ENOBUFS         WSAENOBUFS
// #define ECONNRESET      WSAECONNRESET
// #define ESHUTDOWN       WSAESHUTDOWN
// #define EAFNOSUPPORT    WSAEAFNOSUPPORT
// #define EPROTONOSUPPORT WSAEPROTONOSUPPORT
// #define EINPROGRESS     WSAEINPROGRESS
// #define EISCONN         WSAEISCONN

#ifndef NAME_MAX
#define NAME_MAX 255
#endif
/* These wrappers do nothing special except set the global errno variable if
 * an error occurs (winsock doesn't do this by default). They set errno
 * to unix-like values (i.e. WSAEWOULDBLOCK is mapped to EAGAIN), so code
 * outside of this file "shouldn't" have to worry about winsock specific error
 * handling.
 */
// #define socket(x, y, z)      mingw_socket(x, y, z)
// #define connect(x, y, z)     mingw_connect(x, y, z)
// #define accept(x, y, z)      mingw_accept(x, y, z)
// #define shutdown(x, y)       mingw_shutdown(x, y)
// #define getpeername(x, y, z) mingw_getpeername(x, y, z)

/* Wrapper macros to call misc. functions mingw is missing */
// #define sleep(x)             mingw_sleep(x)
// #define stat(x, y)           mingw_stat(x, y)
//
// #define mkdir(x, y) mkdir(x)

/* Winsock uses int instead of the usual socklen_t */
// typedef int socklen_t;

/* Function prototypes for functions in mingw.c */
// unsigned int mingw_sleep(unsigned int);
// SOCKET  mingw_socket(int, int, int);
// int     mingw_connect(SOCKET, struct sockaddr*, socklen_t);
// SOCKET  mingw_accept(SOCKET, struct sockaddr*, socklen_t *);
// int     mingw_shutdown(SOCKET, int);
// int     mingw_getpeername(SOCKET, struct sockaddr*, socklen_t *);

/* Three socket specific macros */
// #define READ(x, y, z)  mingw_read_socket(x, y, z)
// #define WRITE(x, y, z) mingw_write_socket(x, y, z)
// #define CLOSE(x)       mingw_close_socket(x)
//
// int mingw_read_socket(SOCKET, void *, int);
// int mingw_write_socket(SOCKET, void *, int);
// int mingw_close_socket(SOCKET);
//
// int mingw_setnonblocking(SOCKET, int);
// int mingw_stat(const char*, struct stat*);
#define strerror(e) mingw_strerror(e)

KTORRENT_EXPORT char *mingw_strerror(int error);

#undef ERROR
#undef CopyFile

#endif
