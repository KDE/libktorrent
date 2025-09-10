/*
    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KTSOAP_H
#define KTSOAP_H

#include <QList>
#include <QString>

namespace bt
{
/*!
 * \headerfile torrent/soap.h
 * \author Joris Guisson
 * \brief Empty class for constructing SOAP commands.
 *
 * TODO: why not just use a namespace?
 */
class SOAP
{
public:
    /*!
     * Create a simple UPnP SOAP command without parameters.
     * \param action The name of the action
     * \param service The name of the service
     * \return The command
     */
    static QString createCommand(const QString &action, const QString &service);

    /*!
     * \headerfile torrent/soap.h
     * \brief An argument for a UPnP SOAP command.
     */
    struct Arg {
        QString element;
        QString value;
    };

    /*!
     * Create a UPnP SOAP command with parameters.
     * \param action The name of the action
     * \param service The name of the service
     * \param args Arguments for command
     * \return The command
     */
    static QString createCommand(const QString &action, const QString &service, const QList<Arg> &args);
};

}

#endif
