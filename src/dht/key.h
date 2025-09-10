/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef DHTKEY_H
#define DHTKEY_H

#include <QByteArray>
#include <ktorrent_export.h>
#include <util/sha1hash.h>

namespace dht
{
/*!
 * \headerfile dht/key.h
 * \author Joris Guisson
 * \brief Key in the distributed hash table.
 *
 * Key's in the distributed hash table are just SHA-1 hashes.
 * Key provides all necessary operators to be used as a value.
 */
class KTORRENT_EXPORT Key : public bt::SHA1Hash
{
public:
    /*!
     * Constructor, sets key to 0.
     */
    Key();

    /*!
     * Copy constructor. Seeing that Key doesn't add any data
     * we just pass a SHA1Hash, Key's are automatically covered by this
     * \param k Hash to copy
     */
    Key(const bt::SHA1Hash &k);

    /*!
     * Make a key out of a bytearray
     * \param ba The QByteArray
     */
    Key(const QByteArray &ba);

    /*!
     * Make a key out of a 20 byte array.
     * \param d The array
     */
    Key(const bt::Uint8 *d);

    //! Destructor.
    ~Key() override;

    /*!
     * Create a random key.
     * \return A random Key
     */
    static Key random();

    //! Get the minimum key (all zeros)
    static Key min();

    //! Get the maximum key (all FF)
    static Key max();

    /*!
     * Equality operator.
     * \param other The key to compare
     * \return true if this key is equal to other
     */
    bool operator==(const Key &other) const;

    /*!
     * Inequality operator.
     * \param other The key to compare
     * \return true if this key is not equal to other
     */
    bool operator!=(const Key &other) const;

    /*!
     * Smaller then operator.
     * \param other The key to compare
     * \return rue if this key is smaller then other
     */
    bool operator<(const Key &other) const;

    /*!
     * Smaller then or equal operator.
     * \param other The key to compare
     * \return rue if this key is smaller then or equal to other
     */
    bool operator<=(const Key &other) const;

    /*!
     * Greater then operator.
     * \param other The key to compare
     * \return rue if this key is greater then other
     */
    bool operator>(const Key &other) const;

    /*!
     * Greater then or equal operator.
     * \param other The key to compare
     * \return rue if this key is greater then or equal to other
     */
    bool operator>=(const Key &other) const;

    /*!
     * Divide by a number operator
     */
    Key operator/(int value) const;

    /*!
     * Addition for keys
     * \param a The first key
     * \param b The second key
     */
    friend KTORRENT_EXPORT Key operator+(const Key &a, const Key &b);

    /*!
     * Subtraction for keys
     * \param a The first key
     * \param b The second key
     */
    friend KTORRENT_EXPORT Key operator-(const Key &a, const Key &b);

    /*!
     * Addition for key and a value
     * \param a The key
     * \param value The value
     */
    friend KTORRENT_EXPORT Key operator+(const Key &a, bt::Uint8 value);

    /*!
     * The distance of two keys is the keys xor together.
     * \param a The first key
     * \param b The second key
     * \return a xor b
     */
    static Key distance(const Key &a, const Key &b);

    /*!
     * Calculate the middle between two keys.
     * \param a The first key
     * \param b The second key
     * \return The middle
     */
    static Key mid(const Key &a, const Key &b);
};

}

#endif
