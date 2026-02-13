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
    Uint32 m_num;
    std::unique_ptr<T[]> m_data;

public:
    Array(Uint32 num = 0)
        : m_num(num)
    {
        if (num > 0)
            m_data = std::make_unique<T[]>(num);
    }

    /*!
     * Construct an array by moving the data from \a other.
     *
     * This sets the capacity of \a other to 0 to avoid unneccessary allocations.
     */
    Array(Array &&other) noexcept
        : m_num(other.m_num)
        , m_data(std::move(other.m_data))
    {
        other.m_num = 0;
    }

    /*!
     * Assigns the array by moving the data from \a other.
     *
     * This sets the capacity of \a other to 0 to avoid unneccessary allocations.
     */
    Array &operator=(Array &&other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        m_num = other.m_num;
        m_data = std::move(other.m_data);
        other.m_num = 0;
        return *this;
    }

    ~Array()
    {
    }

    T &operator[](Uint32 i)
    {
        return m_data[i];
    }
    const T &operator[](Uint32 i) const
    {
        return m_data[i];
    }

    operator const T *() const
    {
        return m_data.get();
    }
    operator T *()
    {
        return m_data.get();
    }

    //! Get the number of elements in the array
    [[nodiscard]] Uint32 size() const
    {
        return m_num;
    }

    /*!
     * Fill the array with a value
     * \param val The value
     */
    void fill(T val)
    {
        for (Uint32 i = 0; i < m_num; i++)
            m_data[i] = val;
    }
};

}

#endif
