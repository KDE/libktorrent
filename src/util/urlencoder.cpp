/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "urlencoder.h"

using namespace Qt::Literals::StringLiterals;

namespace bt
{
static const QString hex = u"0123456789ABCDEF"_s;

QString URLEncoder::encode(const char *buf, Uint32 size)
{
    QString res;

    for (Uint32 i = 0; i < size; i++) {
        Uint8 ch = buf[i];
        if (('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || ('0' <= ch && ch <= '9')) {
            // 'A'..'Z'
            res.append(QLatin1Char(ch));
        } else if (ch == ' ') {
            // space
            res.append(QStringLiteral("%20"));
        } else if (ch == '-' || ch == '_' // unreserved
                   || ch == '.' || ch == '!' || ch == '~' || ch == '*' || ch == '\'' || ch == '(' || ch == ')') {
            res.append(QLatin1Char(ch));
        } else {
            // other ASCII as hexadecimal
            res.append('%'_L1);
            res.append(hex[ch / 16]);
            res.append(hex[ch % 16]);
        }
    }
    return res;
}

}
