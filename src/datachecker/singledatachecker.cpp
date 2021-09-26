/*
    SPDX-FileCopyrightText: 2005 Joris Guisson
    joris.guisson@gmail.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "singledatachecker.h"
#include <klocalizedstring.h>
#include <torrent/globals.h>
#include <torrent/torrent.h>
#include <util/array.h>
#include <util/error.h>
#include <util/file.h>
#include <util/functions.h>
#include <util/log.h>

namespace bt
{
SingleDataChecker::SingleDataChecker(bt::Uint32 from, bt::Uint32 to)
    : DataChecker(from, to)
{
}

SingleDataChecker::~SingleDataChecker()
{
}

void SingleDataChecker::check(const QString &path, const Torrent &tor, const QString &, const BitSet &current_status)
{
    // open the file
    Uint32 num_chunks = tor.getNumChunks();
    Uint32 chunk_size = tor.getChunkSize();
    File fptr;
    if (!fptr.open(path, "rb")) {
        throw Error(i18n("Cannot open file %1: %2", path, fptr.errorString()));
    }

    if (from >= tor.getNumChunks())
        from = 0;
    if (to >= tor.getNumChunks())
        to = tor.getNumChunks() - 1;

    // initialize the bitset
    result = BitSet(num_chunks);
    // loop over all chunks
    Array<Uint8> buf(chunk_size);
    TimeStamp last_emitted = bt::Now();
    for (Uint32 i = from; i <= to && !need_to_stop; i++) {
        if (!fptr.eof()) {
            // read the chunk
            Uint32 size = i == num_chunks - 1 ? tor.getLastChunkSize() : tor.getChunkSize();

            fptr.seek(File::BEGIN, (Int64)i * tor.getChunkSize());
            fptr.read(buf, size);
            // generate and test hash
            SHA1Hash h = SHA1Hash::generate(buf, size);
            bool ok = (h == tor.getHash(i));
            result.set(i, ok);
            if (ok && current_status.get(i))
                downloaded++;
            else if (!ok && current_status.get(i))
                failed++;
            else if (!ok && !current_status.get(i))
                not_downloaded++;
            else if (ok && !current_status.get(i))
                found++;
        } else {
            // at end of file so set to default values for a failed chunk
            result.set(i, false);
            if (!current_status.get(i))
                not_downloaded++;
            else
                failed++;
        }

        TimeStamp now = Now();
        if (now - last_emitted > 1000 || i == num_chunks - 1) { // Emit signals once every second
            status(failed, found, downloaded, not_downloaded);
            progress(i - from, from - to + 1);
            last_emitted = now;
        }
    }

    status(failed, found, downloaded, not_downloaded);
}

}
