/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTREQUEST_H
#define BTREQUEST_H

#include <util/constants.h>

namespace bt
{
class PieceDownloader;

/*!
 * \headerfile download/request.h
 * \author Joris Guisson
 * \brief Request of a piece sent to other peers.
 *
 * This class keeps track of a request of a piece.
 * The Request consists of an index (the index of the chunk),
 * offset into the chunk and the length of a piece.
 *
 * The PeerID of the Peer who sent the request is also kept.
 */
class Request
{
public:
    /*!
     * Constructor, set everything to 0.
     */
    inline Request()
        : index(0)
        , off(0)
        , len(0)
        , pd(nullptr)
    {
    }

    /*!
     * Constructor, set the index, offset,length and peer
     * \param index The index of the chunk
     * \param off The offset into the chunk
     * \param len The length of the piece
     * \param pd Pointer to PieceDownloader of the request
     */
    inline Request(Uint32 index, Uint32 off, Uint32 len, PieceDownloader *pd)
        : index(index)
        , off(off)
        , len(len)
        , pd(pd)
    {
    }

    /*!
     * Copy constructor.
     * \param r Request to copy
     */
    inline Request(const Request &r)
        : index(r.index)
        , off(r.off)
        , len(r.len)
        , pd(r.pd)
    {
    }
    ~Request()
    {
    }

    //! Get the index of the chunk
    [[nodiscard]] Uint32 getIndex() const
    {
        return index;
    }

    //! Get the offset into the chunk
    [[nodiscard]] Uint32 getOffset() const
    {
        return off;
    }

    //! Get the length of a the piece
    [[nodiscard]] Uint32 getLength() const
    {
        return len;
    }

    //! Get the sending Peer
    [[nodiscard]] inline PieceDownloader *getPieceDownloader() const
    {
        return pd;
    }

    /*!
     * Assignment operator.
     * \param r The Request to copy
     */
    inline Request &operator=(const Request &r)
    {
        index = r.index;
        off = r.off;
        len = r.len;
        pd = r.pd;
        return *this;
    }

    /*!
     * Compare two requests. Return true if they are the same.
     * This only compares the index,offset and length.
     * \param a The first request
     * \param b The second request
     * \return true if they are equal
     */
    friend inline bool operator==(const Request &a, const Request &b)
    {
        return a.index == b.index && a.len == b.len && a.off == b.off;
    }

private:
    Uint32 index, off, len;
    PieceDownloader *pd;
};

}

#endif
