/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MSEBIGINT_H
#define MSEBIGINT_H

#include <QString>
#include <cstdio>
#include <gmp.h>
#include <util/constants.h>

namespace mse
{
/*!
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * Class which can hold an arbitrary large integer. This will be a very important part of our
 * MSE implementation.
 */
class BigInt
{
public:
    /*!
     * Create a big integer, with num_bits bits.
     * All bits will be set to 0.
     * \param num_bits The number of bits
     */
    BigInt(bt::Uint32 num_bits = 0);

    /*!
     * Create a big integer of a string. The string must be
     * a hexadecimal representation of an integer. For example :
     * 12AFFE123488BBBE123
     *
     * Letters can be upper or lower case. Invalid chars will create an invalid number.
     * \param value The hexadecimal representation of the number
     */
    BigInt(const QString &value);

    /*!
     * Copy constructor.
     * \param bi BigInt to copy
     */
    BigInt(const BigInt &bi);
    ~BigInt();

    /*!
     * Assignment operator.
     * \param bi The BigInt to copy
     * \return *this
     */
    BigInt &operator=(const BigInt &bi);

    /*!
     * Calculates
     * (x ^ e) mod d
     * ^ is power
     */
    static BigInt powerMod(const BigInt &x, const BigInt &e, const BigInt &d);

    //! Make a random BigInt
    static BigInt random();

    //! Export the bigint ot a buffer
    bt::Uint32 toBuffer(bt::Uint8 *buf, bt::Uint32 max_size) const;

    //! Make a BigInt out of a buffer
    static BigInt fromBuffer(const bt::Uint8 *buf, bt::Uint32 size);

private:
    mpz_t val;
};

}

#endif
