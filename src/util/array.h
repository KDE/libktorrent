/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTARRAY_H
#define BTARRAY_H

#include "constants.h"
#include <ktorrent_export.h>
#include <memory>

namespace bt
{
/*!
 * \author Joris Guisson
 *
 * \brief Fixed capacity dynamically allocated buffer.
 */
template<class T> class KTORRENT_EXPORT Array
{
    Uint32 num;
    std::unique_ptr<T[]> data;

public:
    Array(Uint32 num = 0)
        : num(num)
    {
        if (num > 0)
            data = std::make_unique<T[]>(num);
    }

    /*!
     * Construct an array by moving the data from \a other.
     *
     * This sets the capacity of \a other to 0 to avoid unneccessary allocations.
     */
    Array(Array &&other)
        : num(other.num)
        , data(std::move(other.data))
    {
        other.num = 0;
    }

    /*!
     * Assigns the array by moving the data from \a other.
     *
     * This sets the capacity of \a other to 0 to avoid unneccessary allocations.
     */
    Array &operator=(Array &&other)
    {
        if (this == &other) {
            return *this;
        }
        num = other.num;
        data = std::move(other.data);
        other.num = 0;
        return *this;
    }

    ~Array()
    {
    }

    T &operator[](Uint32 i)
    {
        return data[i];
    }
    const T &operator[](Uint32 i) const
    {
        return data[i];
    }

    operator const T *() const
    {
        return data.get();
    }
    operator T *()
    {
        return data.get();
    }

    //! Get the number of elements in the array
    Uint32 size() const
    {
        return num;
    }

    /*!
     * Fill the array with a value
     * \param val The value
     */
    void fill(T val)
    {
        for (Uint32 i = 0; i < num; i++)
            data[i] = val;
    }
};

}

#endif
