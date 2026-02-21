/*
    SPDX-FileCopyrightText: 2012 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTILS_H
#define UTILS_H

#include <optional>

#include <net/socketdevice.h>
#include <util/constants.h>

//! Generate a random size between min_size and max_size
bt::Uint64 RandomSize(bt::Uint64 min_size, bt::Uint64 max_size);

struct SocketPair {
    std::unique_ptr<net::SocketDevice> reader;
    std::unique_ptr<net::SocketDevice> writer;
};

std::optional<SocketPair> CreateSocketPair(int ip_version = 6);

#endif // UTILS_H
