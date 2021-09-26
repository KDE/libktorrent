/*
    SPDX-FileCopyrightText: 2012 Digia Plc and /or its subsidiary(-ies). <http://www.qt-project.org/legal>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
