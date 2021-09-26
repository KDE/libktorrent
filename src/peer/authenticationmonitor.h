/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTAUTHENTICATIONMONITOR_H
#define BTAUTHENTICATIONMONITOR_H

#include <ktorrent_export.h>
#include <list>
#include <net/poll.h>
#include <vector>

namespace bt
{
class AuthenticateBase;

/**
    @author Joris Guisson <joris.guisson@gmail.com>

    Monitors ongoing authentication attempts. This class is a singleton.
*/
class KTORRENT_EXPORT AuthenticationMonitor : public net::Poll
{
    std::list<AuthenticateBase *> auths;

    static AuthenticationMonitor self;

    AuthenticationMonitor();

public:
    ~AuthenticationMonitor() override;

    /**
     * Add a new AuthenticateBase object.
     * @param s
     */
    void add(AuthenticateBase *s);

    /**
     * Remove an AuthenticateBase object
     * @param s
     */
    void remove(AuthenticateBase *s);

    /**
     * Check all AuthenticateBase objects.
     */
    void update();

    /**
     * Clear all AuthenticateBase objects, also delets them
     */
    void clear();

    /**
     * Shutdown the authentication manager
     */
    void shutdown();

    static AuthenticationMonitor &instance()
    {
        return self;
    }

private:
    void handleData();
};

}

#endif
