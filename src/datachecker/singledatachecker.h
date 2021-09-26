/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSINGLEDATACHECKER_H
#define BTSINGLEDATACHECKER_H

#include "datachecker.h"

namespace bt
{
/**
 * @author Joris Guisson
 *
 * Data checker for single file torrents.
 */
class KTORRENT_EXPORT SingleDataChecker : public DataChecker
{
public:
    SingleDataChecker(bt::Uint32 from, bt::Uint32 to);
    ~SingleDataChecker() override;

    void check(const QString &path, const Torrent &tor, const QString &dnddir, const BitSet &current_status) override;
};

}

#endif
