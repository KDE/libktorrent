/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTFILE_H
#define BTFILE_H

#include "constants.h"
#include <cstdio>
#include <ktorrent_export.h>

#include <QString>

namespace bt
{
/*!
 * \author Joris Guisson
 * \brief Wrapper class for stdio's FILE.
 */
class KTORRENT_EXPORT File
{
    FILE *fptr;
    QString file;

public:
    /*!
     * Constructor.
     */
    File();

    /*!
     * Destructor, closes the file.
     */
    virtual ~File();

    File(const File &) = delete;
    File &operator=(const File &) = delete;

    /*!
     * Move constructor.
     */
    File(File &&other) noexcept;

    /*!
     * Move assignment operator.
     */
    File &operator=(File &&other) noexcept;

    /*!
     * Open the file similar to fopen
     * \param file Filename
     * \param mode Mode
     * \return true upon succes
     */
    bool open(const QString &file, const QString &mode);

    /*!
     * Close the file.
     */
    void close();

    /*!
     * Flush the file.
     */
    void flush();

    /*!
     * Write a bunch of data. If anything goes wrong
     * an Error will be thrown.
     * \param buf The data
     * \param size Size of the data
     * \return The number of bytes written
     */
    Uint32 write(const void *buf, Uint32 size);

    /*!
     * Read a bunch of data. If anything goes wrong
     * an Error will be thrown.
     * \param buf The buffer to store the data
     * \param size Size of the buffer
     * \return The number of bytes read
     */
    Uint32 read(void *buf, Uint32 size);

    enum SeekPos {
        BEGIN,
        END,
        CURRENT,
    };

    /*!
     * Seek in the file.
     * \param from Position to seek from
     * \param num Number of bytes to move
     * \return New position
     */
    Uint64 seek(SeekPos from, Int64 num);

    //! Check to see if we are at the end of the file.
    bool eof() const;

    //! Get the current position in the file.
    Uint64 tell() const;

    //! Get the error string.
    QString errorString() const;
};

}

#endif
