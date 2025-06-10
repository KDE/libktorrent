/*
    SPDX-FileCopyrightText: 2008 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2008 Ivan Vasic <ivasic@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPIECEDATA_H
#define BTPIECEDATA_H

#include <QSharedDataPointer>
#include <ktorrent_export.h>
#include <util/constants.h>
#ifndef Q_OS_WIN
#include <util/signalcatcher.h>
#endif
#include <diskio/cachefile.h>

namespace bt
{
class File;
class Chunk;
class SHA1Hash;
class SHA1HashGen;

/*!
    \brief Holds the data of a piece of a chunk.

    It has a reference counter.
*/
class KTORRENT_EXPORT PieceData : public QSharedData, public MMappeable
{
public:
    PieceData(Chunk *chunk, Uint32 off, Uint32 len, Uint8 *ptr, CacheFile::Ptr cache_file, bool read_only);
    ~PieceData() override;

    //! Unload the piece
    void unload();

    //! Is it a mapped into memory
    bool mapped() const
    {
        return cache_file != nullptr;
    }

    //! Is this writeable
    bool writeable() const
    {
        return !read_only;
    }

    //! Get the offset of the piece in the chunk
    Uint32 offset() const
    {
        return off;
    }

    //! Get the length of the piece
    Uint32 length() const
    {
        return len;
    }

    //! Get a pointer to the data
    Uint8 *data()
    {
        return ptr;
    }

    //! Check if the data pointer is OK
    bool ok() const
    {
        return ptr != nullptr;
    }

    //! Set the data pointer
    void setData(Uint8 *p)
    {
        ptr = p;
    }

    //! Get the parent chunk of the piece
    Chunk *parentChunk()
    {
        return chunk;
    }

    /*!
        Write data into the PieceData. This function should always be used
        for writing into a PieceData object, as it protects against bus errors.
        \param buf The buffer to write
        \param buf_size Size of the buffer
        \param off Offset to write
        \return The number of bytes written
        \throw BusError When writing results in a SIGBUS
    */
    Uint32 write(const Uint8 *buf, Uint32 buf_size, Uint32 off = 0);

    /*!
        Read data from the PieceData. This function should always be used
        for reading from a PieceData object, as it protects against bus errors.
        \param buf The buffer to read into
        \param to_read Amount of bytes to read
        \param off Offset in the PieceData to start reading from
        \return The number of bytes read
        \throw BusError When reading results in a SIGBUS
     */
    Uint32 read(Uint8 *buf, Uint32 to_read, Uint32 off = 0);

    /*!
        Save PieceData to a File. This function protects against bus errors.
        \param file The file to write to
        \param size Size to write
        \param off Offset in PieceData to write from
        \return The number of bytes written
        \throw BusError When writing results in a SIGBUS
    */
    Uint32 writeToFile(File &file, Uint32 size, Uint32 off = 0);

    /*!
        Read PieceData from a File. This function protects against bus errors.
        \param file The file to read from
        \param size Size to read
        \param off Offset in PieceData to write into
        \return The number of bytes read
        \throw BusError When reading results in a SIGBUS
    */
    Uint32 readFromFile(File &file, Uint32 size, Uint32 off = 0);

    /*!
        Update a SHA1HashGen with this PieceData. This function protects against bus errors.
        \param hg The SHA1HashGen to update
        \throw BusError When reading results in a SIGBUS
     */
    void updateHash(SHA1HashGen &hg);

    /*!
        Generate a SHA1Hash of this PieceData. This function protects against bus errors.
        \return The SHA1 hash
        \throw BusError When reading results in a SIGBUS
     */
    SHA1Hash generateHash() const;

    typedef QExplicitlySharedDataPointer<PieceData> Ptr;

    //! Is the piece in use by somebody else then the cache
    bool inUse() const
    {
        return ref > 1;
    }

private:
    void unmapped() override;

private:
    Chunk *chunk;
    Uint32 off;
    Uint32 len;
    Uint8 *ptr;
    CacheFile::Ptr cache_file;
    bool read_only;
};

}

#endif
