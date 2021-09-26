/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTDATACHECKER_H
#define BTDATACHECKER_H

#include <QObject>
#include <ktorrent_export.h>
#include <util/bitset.h>

class QString;

namespace bt
{
class Torrent;

/**
 * @author Joris Guisson
 *
 * Checks which data is downloaded, given a torrent and a file or directory containing
 * files of the torrent.
 */
class KTORRENT_EXPORT DataChecker : public QObject
{
    Q_OBJECT
public:
    DataChecker(bt::Uint32 from, bt::Uint32 to);
    ~DataChecker() override;

    /**
     * Check to see which chunks have been downloaded of a torrent, and which chunks fail.
     * The corresponding bitsets should be filled with this information.
     * If anything goes wrong and Error should be thrown.
     * @param path path to the file or dir (this needs to end with the name suggestion of the torrent)
     * @param tor The torrent
     * @param dnddir DND dir, optional argument if we know this
     * @param current_status Current status of the torrent
     */
    virtual void check(const QString &path, const Torrent &tor, const QString &dnddir, const BitSet &current_status) = 0;

    /**
     * Get the BitSet representing all the downloaded chunks and which is the result of the data check.
     */
    const BitSet &getResult() const
    {
        return result;
    }

    /// Stop an ongoing check
    void stop()
    {
        need_to_stop = true;
    }

Q_SIGNALS:
    /**
     * Emitted when a chunk has been proccessed.
     * @param num The number processed
     * @param total The total number of pieces to process
     */
    void progress(quint32 num, quint32 total);

    /**
     * Emitted when a failed or dowloaded chunk is found.
     * @param num_failed The number of failed chunks
     * @param num_found The number of found chunks
     * @param num_downloaded Number of downloaded chunks
     * @param num_not_downloaded Number of not downloaded chunks
     */
    void status(quint32 num_failed, quint32 num_found, quint32 num_downloaded, quint32 num_not_downloaded);

protected:
    BitSet result;
    Uint32 failed, found, downloaded, not_downloaded;
    bool need_to_stop;
    bt::Uint32 from;
    bt::Uint32 to;
};

}

#endif
