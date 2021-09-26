/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "value.h"
#include <qtextcodec.h>

namespace bt
{
Value::Value()
    : type(INT)
    , ival(0)
    , big_ival(0)
{
}

Value::Value(int val)
    : type(INT)
    , ival(val)
    , big_ival(val)
{
}

Value::Value(Int64 val)
    : type(INT64)
    , big_ival(val)
{
}

Value::Value(const QByteArray &val)
    : type(STRING)
    , ival(0)
    , strval(val)
    , big_ival(0)
{
}

Value::Value(const Value &val)
    : type(val.type)
    , ival(val.ival)
    , strval(val.strval)
    , big_ival(val.big_ival)
{
}

Value::~Value()
{
}

QString Value::toString(QTextCodec *tc) const
{
    if (!tc)
        return toString();
    else
        return tc->toUnicode(strval);
}

Value &Value::operator=(const Value &val)
{
    type = val.type;
    ival = val.ival;
    strval = val.strval;
    big_ival = val.big_ival;
    return *this;
}

Value &Value::operator=(Int32 val)
{
    type = INT;
    ival = val;
    big_ival = val;
    return *this;
}

Value &Value::operator=(Int64 val)
{
    type = INT64;
    big_ival = val;
    return *this;
}

Value &Value::operator=(const QByteArray &val)
{
    type = STRING;
    strval = val;
    big_ival = 0;
    return *this;
}

}
