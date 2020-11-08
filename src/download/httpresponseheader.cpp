/***************************************************************************
 *   Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).             *
 *   Contact: http://www.qt-project.org/legal                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "httpresponseheader.h"

bool HttpResponseHeader::parseLine(const QString &line, int number)
{
    if (number != 0) {
        int i = line.indexOf(QLatin1Char(':'));
        if (i == -1)
            return false;

        values[line.leftRef(i).trimmed().toString().toLower()] = line.midRef(i + 1).trimmed().toString();

        return true;
    }

    QString l = line.simplified();
    if (l.length() < 10)
        return false;

    if (l.startsWith(QLatin1String("HTTP/")) && l[5].isDigit() && l[6] == QLatin1Char('.') &&
        l[7].isDigit() && l[8] == QLatin1Char(' ') && l[9].isDigit()) {
        _majVer = l[5].toLatin1() - '0';
        _minVer = l[7].toLatin1() - '0';

        int pos = l.indexOf(QLatin1Char(' '), 9);
        if (pos != -1) {
            _reasonPhr = l.mid(pos + 1);
            _statCode = l.midRef(9, pos - 9).toInt();
        } else {
            _statCode = l.midRef(9).toInt();
            _reasonPhr.clear();
        }
    } else {
        return false;
    }

    return true;
}

bool HttpResponseHeader::parse(const QString &str)
{
    QStringList lst;
    int pos = str.indexOf(QLatin1Char('\n'));
    if (pos > 0 && str.at(pos - 1) == QLatin1Char('\r'))
        lst = str.trimmed().split(QLatin1String("\r\n"));
    else
        lst = str.trimmed().split(QLatin1String("\n"));
    lst.removeAll(QString()); // No empties

    if (lst.isEmpty())
        return true;

    QStringList lines;
    for (auto it = lst.begin(); it != lst.end(); ++it) {
        if (!(*it).isEmpty()) {
            if ((*it)[0].isSpace()) {
                if (!lines.isEmpty()) {
                    lines.last() += QLatin1Char(' ') + (*it).trimmed();
                }
            } else {
                lines.append((*it));
            }
        }
    }

    int number = 0;
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (!parseLine(*it, number++)) {
            return false;
        }
    }
    return true;
}

