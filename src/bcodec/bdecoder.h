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

/**
 * @author Joris Guisson
 * @brief Decodes b-encoded data
 *
 * Class to decode b-encoded data.
 */
class KTORRENT_EXPORT BDecoder
{
    QByteArray data;
    Uint32 pos;
    bool verbose;
    int level;

public:
    /**
     * Constructor, passes in the data to decode.
     * @param ptr Pointer to the data
     * @param size Size of the data
     * @param verbose Verbose output to the log
     * @param off Offset to start parsing
     */
    BDecoder(const Uint8 *ptr, Uint32 size, bool verbose, Uint32 off = 0);

    /**
     * Constructor, passes in the data to decode.
     * @param data The data
     * @param verbose Verbose output to the log
     * @param off Offset to start parsing
     */
    BDecoder(const QByteArray &data, bool verbose, Uint32 off = 0);
    virtual ~BDecoder();

    /**
     * Decode the data, the root node gets
     * returned. (Note that the caller must delete this node)
     * @return The root node
     */
    BNode *decode();

    /**
     * Decode the data, the root list node gets
     * returned. (Note that the caller must delete this node)
     * @return The root node
     */
    BListNode *decodeList();

    /**
     * Decode the data, the root dict node gets
     * returned. (Note that the caller must delete this node)
     * @return The root node
     */
    BDictNode *decodeDict();

    /// Get the current position in the data
    Uint32 position() const
    {
        return pos;
    }

private:
    void debugMsg(const QString &msg);

private:
    BDictNode *parseDict();
    BListNode *parseList();
    BValueNode *parseInt();
    BValueNode *parseString();
};

}

#endif
