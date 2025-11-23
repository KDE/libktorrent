/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTVALUE_H
#define BTVALUE_H

#include <QString>

#include <ktorrent_export.h>
#include <util/constants.h>
namespace bt
{
/*!
 * \headerfile bcodec/value.h
 * \author Joris Guisson
 * \brief Acts like a union for bencoded data types.
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
    Value(QByteArrayView val);
    Value(const Value &val);
    ~Value();

    Value &operator=(const Value &val);
    Value &operator=(Int32 val);
    Value &operator=(Int64 val);
    Value &operator=(QByteArrayView val);

    [[nodiscard]] Type getType() const
    {
        return type;
    }
    [[nodiscard]] Int32 toInt() const
    {
        return ival;
    }
    [[nodiscard]] Int64 toInt64() const
    {
        return big_ival;
    }
    [[nodiscard]] QString toString() const
    {
        return QString::fromUtf8(strval);
    }

    /*!
     * Returns a deep copy of the string.
     */
    [[nodiscard]] QByteArray toByteArray() const
    {
        return strval.toByteArray();
    }

    /*!
     * Returns a view over the byte string
     */
    [[nodiscard]] QByteArrayView toByteArrayView() const
    {
        return strval;
    }

private:
    Type type;
    Int32 ival;
    QByteArrayView strval;
    Int64 big_ival;
};
}

#endif
