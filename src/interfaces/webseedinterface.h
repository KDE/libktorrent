/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTWEBSEEDINTERFACE_H
#define BTWEBSEEDINTERFACE_H

#include <QUrl>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
/*!
    Interface for WebSeeds
*/
class KTORRENT_EXPORT WebSeedInterface
{
public:
    WebSeedInterface(const QUrl &url, bool user);
    virtual ~WebSeedInterface();

    //! Disable or enable the webseed
    virtual void setEnabled(bool on);

    //! Whether or not the webseed is enabled
    bool isEnabled() const
    {
        return enabled;
    }

    //! Get the URL of the webseed
    const QUrl &getUrl() const
    {
        return url;
    }

    //! Get how much data was downloaded
    Uint64 getTotalDownloaded() const
    {
        return total_downloaded;
    }

    //! Get the present status in string form
    QString getStatus() const
    {
        return status;
    }

    //! Get the current download rate in bytes per sec
    virtual Uint32 getDownloadRate() const = 0;

    //! Whether or not this webseed was user created
    bool isUserCreated() const
    {
        return user;
    }

protected:
    QUrl url;
    Uint64 total_downloaded;
    QString status;
    bool user;
    bool enabled;
};

}

#endif
