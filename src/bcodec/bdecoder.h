/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTBDECODER_H
#define BTBDECODER_H

#include <QString>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
class BNode;
class BListNode;
class BDictNode;
class BValueNode;

/*!
 * \headerfile bcodec/bdecoder.h
 * \author Joris Guisson
 * \brief Decodes bencoded data.
 */
class KTORRENT_EXPORT BDecoder
{
    QByteArrayView data;
    Uint32 pos = 0;
    bool verbose;
    int level = 0;

public:
    /*!
     * Constructs a BDecoder and initializes it with \a data to decode.
     *
     * If \a verbose is true, debug output is written to the log.
     */
    BDecoder(QByteArrayView data, bool verbose);
    virtual ~BDecoder();

    /*!
     * Decode the data, the root node gets returned.
     * \return The root node
     */
    std::unique_ptr<BNode> decode();

    /*!
     * Decode the data, the root list node gets returned.
     * \return The root node
     */
    std::unique_ptr<BListNode> decodeList();

    /*!
     * Decode the data, the root dict node gets returned.
     * \return The root node
     */
    std::unique_ptr<BDictNode> decodeDict();

    //! Get the current position in the data
    [[nodiscard]] Uint32 position() const
    {
        return pos;
    }

private:
    void debugMsg(const QString &msg);

private:
    std::unique_ptr<BDictNode> parseDict();
    std::unique_ptr<BListNode> parseList();
    std::unique_ptr<BValueNode> parseInt();
    std::unique_ptr<BValueNode> parseString();
};

}

#endif
