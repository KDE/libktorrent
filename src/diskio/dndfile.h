/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTDNDFILE_H
#define BTDNDFILE_H

#include <QString>
#include <QtClassHelperMacros>
#include <util/constants.h>

namespace bt
{
class TorrentFile;

/*!
 * \headerfile diskio/dndfile.h
 * \author Joris Guisson <joris.guisson@gmail.com>
 *
 * \brief Special file where we keep the first and last chunk of a file which is marked as do not download.
 *
 * The first and last chunk of a file will most certainly be partial chunks.
 */
class DNDFile
{
public:
    DNDFile(const QString &path, const TorrentFile *tf, Uint32 chunk_size);
    ~DNDFile();

    Q_DISABLE_COPY_MOVE(DNDFile);

    //! Change the path of the file
    void changePath(const QString &npath);

    /*!
     * CHeck integrity of the file, create it if it doesn't exist.
     */
    void checkIntegrity();

    /*!
     * Read the (partial)first chunk into a buffer.
     * \param buf The buffer
     * \param off Offset off chunk
     * \param size How many bytes to read of the first chunk
     */
    Uint32 readFirstChunk(Uint8 *buf, Uint32 off, Uint32 size);

    /*!
     * Read the (partial)last chunk into a buffer.
     * \param buf The buffer
     * \param off Offset off chunk
     * \param size How many bytes to read of the last chunk
     */
    Uint32 readLastChunk(Uint8 *buf, Uint32 off, Uint32 size);

    /*!
     * Write the partial first chunk.
     * \param buf The buffer
     * \param off Offset into partial chunk
     * \param size Size to write
     */
    void writeFirstChunk(const Uint8 *buf, Uint32 off, Uint32 size);

    /*!
     * Write the partial last chunk.
     * \param buf The buffer
     * \param off Offset into partial chunk
     * \param size Size to write
     */
    void writeLastChunk(const Uint8 *buf, Uint32 off, Uint32 size);

private:
    void create();

private:
    QString path;
    Uint32 first_size;
    Uint32 last_size;
};

}

#endif
