/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTTORRENTCREATOR_H
#define BTTORRENTCREATOR_H

#include "torrent.h"
#include <QStringList>
#include <QThread>
#include <ktorrent_export.h>
#include <util/sha1hash.h>

namespace bt
{
class BEncoder;
class TorrentControl;

/*!
 * \author Joris Guisson
 * \brief Generates torrent files.
 *
 * It also allows to create a TorrentControl object, so
 * that we immediately can start to share the torrent.
 */
class KTORRENT_EXPORT TorrentCreator : public QThread
{
    Q_OBJECT

    // input values
    QString target;
    QStringList trackers;
    QList<QUrl> webseeds;
    int chunk_size;
    QString name, comments;
    // calculated values
    Uint32 num_chunks;
    Uint64 last_size;
    QList<TorrentFile> files;
    QList<SHA1Hash> hashes;
    //
    Uint32 cur_chunk;
    bool priv;
    Uint64 tot_size;
    bool decentralized;
    bool stopped;

public:
    /*!
     * Constructor.
     * \param target The file or directory to make a torrent of
     * \param trackers A list of tracker urls
     * \param webseeds A list of webseed urls
     * \param chunk_size The size of each chunk
     * \param name The name suggestion
     * \param comments The comments field of the torrent
     * \param priv Private torrent or not
     * \param decentralized If this is true then trackers are treated
     * as DHT nodes and are written to the \a nodes section of the file.
     */
    TorrentCreator(const QString &target,
                   const QStringList &trackers,
                   const QList<QUrl> &webseeds,
                   Uint32 chunk_size,
                   const QString &name,
                   const QString &comments,
                   bool priv,
                   bool decentralized);
    ~TorrentCreator() override;

    //! Get the number of chunks
    [[nodiscard]] Uint32 getNumChunks() const
    {
        return num_chunks;
    }

    [[nodiscard]] Uint32 getCurrentChunk() const
    {
        return cur_chunk;
    }

    /*!
     * Save the torrent file.
     * \param url Filename
     * \throw Error if something goes wrong
     */
    void saveTorrent(const QString &url);

    /*!
     * Make a TorrentControl object for this torrent.
     * This will also create the files :
     * data_dir/index
     * data_dir/torrent
     * data_dir/cache (symlink to target)
     * \param data_dir The data directory
     * \throw Error if something goes wrong
     * \return The newly created object
     */
    TorrentControl *makeTC(const QString &data_dir);

    //! Stop the thread
    void stop()
    {
        stopped = true;
    }

private:
    void saveInfo(BEncoder &enc);
    void saveFile(BEncoder &enc, const TorrentFile &file);
    void savePieces(BEncoder &enc);
    void buildFileList(const QString &dir);
    bool calcHashSingle();
    bool calcHashMulti();
    void run() override;
    bool calculateHash();
};

}

#endif
