/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "urlencoder.h"

using namespace Qt::Literals::StringLiterals;

namespace bt
{
static const QString hex = u"0123456789ABCDEF"_s;

QString URLEncoder::encode(QByteArrayView buf)
{
    QString res;
    res.reserve(buf.size() * 3); // Worst case

    for (const char ch : buf) {
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
            res.append(hex[static_cast<Uint8>(ch) / 16]);
            res.append(hex[static_cast<Uint8>(ch) % 16]);
        }
    }
    return res;
}

}
