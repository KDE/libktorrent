/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTURLENCODER_H
#define BTURLENCODER_H

#include "constants.h"
#include <QByteArrayView>
#include <QString>

namespace bt
{
/*!
 * \headerfile util/urlencoder.h
 * \author Joris Guisson
 * \brief Provides a function to percent-encode a byte array.
 *
 * TODO: why not just a namespace?
 */
class URLEncoder
{
public:
    static QString encode(QByteArrayView buf);
};

}

#endif
