/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BTURLENCODER_H
#define BTURLENCODER_H

#include "constants.h"
#include <QString>

namespace bt
{
/**
@author Joris Guisson
*/
class URLEncoder
{
public:
    static QString encode(const char *buf, Uint32 size);
};

}

#endif
