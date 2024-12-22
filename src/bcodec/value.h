/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTVALUE_H
#define BTVALUE_H

#include <ktorrent_export.h>
#include <qstring.h>
#include <util/constants.h>
namespace bt
{
/**
@author Joris Guisson
*/
class KTORRENT_EXPORT Value
{
public:
    enum Type {
        STRING,
        INT,
        INT64,
    };

    Value();
    Value(int val);
    Value(Int64 val);
    Value(const QByteArray &val);
    Value(const Value &val);
    ~Value();

    Value &operator=(const Value &val);
    Value &operator=(Int32 val);
    Value &operator=(Int64 val);
    Value &operator=(const QByteArray &val);

    Type getType() const
    {
        return type;
    }
    Int32 toInt() const
    {
        return ival;
    }
    Int64 toInt64() const
    {
        return big_ival;
    }
    QString toString() const
    {
        return QString::fromUtf8(strval);
    }
    QByteArray toByteArray() const
    {
        return strval;
    }

private:
    Type type;
    Int32 ival;
    QByteArray strval;
    Int64 big_ival;
};
}

#endif
