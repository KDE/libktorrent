/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTBENCODER_H
#define BTBENCODER_H

#include <ktorrent_export.h>
#include <util/file.h>

class QIODevice;

namespace bt
{
class File;

/*!
 * \author Joris Guisson
 *
 * Interface for classes which wish to receive the output from a BEncoder.
 */
class KTORRENT_EXPORT BEncoderOutput
{
public:
    virtual ~BEncoderOutput()
    {
    }
    /*!
     * Write a string of characters.
     * \param str The string
     * \param len The length of the string
     */
    virtual void write(const char *str, Uint32 len) = 0;
};

/*!
 * Writes the output of a bencoder to a file
 */
class KTORRENT_EXPORT BEncoderFileOutput : public BEncoderOutput
{
    File *fptr;

public:
    BEncoderFileOutput(File *fptr);

    void write(const char *str, Uint32 len) override;
};

/*!
 * Write the output of a BEncoder to a QByteArray
 */
class KTORRENT_EXPORT BEncoderBufferOutput : public BEncoderOutput
{
    QByteArray &data;
    Uint32 ptr;

public:
    BEncoderBufferOutput(QByteArray &data);

    void write(const char *str, Uint32 len) override;
};

class KTORRENT_EXPORT BEncoderIODeviceOutput : public BEncoderOutput
{
    QIODevice *dev;

public:
    BEncoderIODeviceOutput(QIODevice *dev);

    void write(const char *str, Uint32 len) override;
};

/*!
 * \author Joris Guisson
 * \brief Helper class to b-encode stuff.
 *
 * This class b-encodes data. For more details about b-encoding, see
 * the BitTorrent protocol docs. The data gets written to a BEncoderOutput
 * thing.
 */
class KTORRENT_EXPORT BEncoder
{
    BEncoderOutput *out;
    bool del;

public:
    /*!
     * Constructor, output gets written to a file.
     * \param fptr The File to write to
     */
    BEncoder(File *fptr);

    /*!
     * Constructor, output gets written to a BEncoderOutput object.
     * \param out The BEncoderOutput
     */
    BEncoder(BEncoderOutput *out);

    /*!
     * Constructor, output gets written to a QIODevice object.
     * \param dev The QIODevice
     */
    BEncoder(QIODevice *dev);

    virtual ~BEncoder();

    /*!
     * Begin a dictionary.Should have a corresponding end call.
     */
    void beginDict();

    /*!
     * Begin a list. Should have a corresponding end call.
     */
    void beginList();

    template<class T> void write(const QByteArray &key, T val)
    {
        write(key);
        write(val);
    }

    /*!
     * Write a boolean (is encoded as an intà
     * \param val
     */
    void write(bool val);

    /*!
     * Write a float
     * \param val
     */
    void write(float val);

    /*!
     * Write an int
     * \param val
     */
    void write(Uint32 val);

    /*!
     * Write an int64
     * \param val
     */
    void write(Uint64 val);

private:
    /*!
     * Write a string
     * \param str
     */
    void write(const char *str);

public:
    /*!
     * Write a QByteArray
     * \param data
     */
    void write(const QByteArray &data);

    /*!
     * Write a data array
     * \param data
     * \param size of data
     */
    void write(const Uint8 *data, Uint32 size);

    /*!
     * End a beginDict or beginList call.
     */
    void end();
};

}

#endif
