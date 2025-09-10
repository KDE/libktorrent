/*
    SPDX-FileCopyrightText: 2005-2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTSTATSFILE_H
#define BTSTATSFILE_H

#include <KSharedConfig>
#include <ktorrent_export.h>
#include <util/constants.h>

namespace bt
{
/*!
 * \headerfile torrent/statsfile.h
 * \author Ivan Vasic <ivasic@gmail.com>
 * \brief Loads and stores torrent stats in a file.
 */
class KTORRENT_EXPORT StatsFile
{
public:
    /*!
     * \brief A constructor.
     * Constructs StatsFile object and calls readSync().
     */
    StatsFile(const QString &filename);
    virtual ~StatsFile();

    QString readString(const QString &key);

    Uint64 readUint64(const QString &key);
    bool readBoolean(const QString &key);
    int readInt(const QString &key);
    unsigned long readULong(const QString &key);
    float readFloat(const QString &key);

    void write(const QString &key, const QString &value);

    //! Sync with disk
    void sync();

    /*!
     * See if there is a key in the stats file
     * \param key The key
     * \return true if key is in the stats file
     */
    [[nodiscard]] bool hasKey(const QString &key) const;

private:
    KSharedConfigPtr cfg;
};
}

#endif
