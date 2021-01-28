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

#include <QMap>
#include <QString>

class HttpResponseHeader
{
public:
    HttpResponseHeader(const QString &str)
    {
        parse(str);
    }
    int statusCode()
    {
        return _statCode;
    }
    QString reasonPhrase()
    {
        return _reasonPhr;
    }
    QString value(const QString &key) const
    {
        return values[key.toLower()];
    }
    bool hasKey(const QString &key) const
    {
        return values.contains(key.toLower());
    }

private:
    bool parse(const QString &);
    bool parseLine(const QString &line, int number);

    QMap<QString, QString> values;
    int _majVer;
    int _minVer;
    int _statCode;
    QString _reasonPhr;
};
