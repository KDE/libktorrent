/*
    SPDX-FileCopyrightText: 2005 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "exitoperation.h"

namespace bt
{
ExitOperation::ExitOperation()
{
}

ExitOperation::~ExitOperation()
{
}

ExitJobOperation::ExitJobOperation(KJob *j)
{
    connect(j, &KJob::result, this, &ExitJobOperation::onResult);
}

ExitJobOperation::~ExitJobOperation()
{
}

void ExitJobOperation::onResult(KJob *)
{
    operationFinished(this);
}

}

#include "moc_exitoperation.cpp"
