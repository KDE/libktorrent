/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTPIECE_H
#define BTPIECE_H

#include "request.h"

namespace bt
{
/*!
 * \author Joris Guisson
 * \brief A piece in a torrent.
 */
class Piece : public Request
{
public:
    inline Piece(Uint32 index, Uint32 off, Uint32 len, PieceDownloader *pd, const Uint8 *data)
        : Request(index, off, len, pd)
        , data(data)
    {
    }
    ~Piece()
    {
    }

    const Uint8 *getData() const
    {
        return data;
    }

private:
    const Uint8 *data;
};

}

#endif
