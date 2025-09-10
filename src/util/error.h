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
    \headerfile util/error.h
    \author Joris Guisson
    \brief Exception thrown whenever an unrecoverable error occurs.
*/
class KTORRENT_EXPORT Error
{
    QString msg;

public:
    Error(const QString &msg);
    virtual ~Error();

    [[nodiscard]] QString toString() const
    {
        return msg;
    }
};

/*!
 * \headerfile util/error.h
 * \brief Exception thrown when the user is a dummy.
 *
 * TODO: formalize when this should be used instead of Error
 */
class KTORRENT_EXPORT Warning
{
    QString msg;

public:
    Warning(const QString &msg);
    virtual ~Warning();

    [[nodiscard]] QString toString() const
    {
        return msg;
    }
};
}

#endif
