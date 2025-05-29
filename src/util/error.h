/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTERROR_H
#define BTERROR_H

#include <QString>
#include <ktorrent_export.h>

namespace bt
{
/*!
    \author Joris Guisson
*/
class KTORRENT_EXPORT Error
{
    QString msg;

public:
    Error(const QString &msg);
    virtual ~Error();

    QString toString() const
    {
        return msg;
    }
};

class KTORRENT_EXPORT Warning
{
    QString msg;

public:
    Warning(const QString &msg);
    virtual ~Warning();

    QString toString() const
    {
        return msg;
    }
};
}

#endif
