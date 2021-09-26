/*
    SPDX-FileCopyrightText: 2008 Joris Guisson and Ivan Vasic
    joris.guisson@gmail.com
    ivasic@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "webseedinterface.h"

namespace bt
{
WebSeedInterface::WebSeedInterface(const QUrl &url, bool user)
    : url(url)
    , total_downloaded(0)
    , user(user)
    , enabled(true)
{
}

WebSeedInterface::~WebSeedInterface()
{
}

void WebSeedInterface::setEnabled(bool on)
{
    enabled = on;
}

}
