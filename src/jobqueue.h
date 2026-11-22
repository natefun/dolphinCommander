/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include "dolphin_export.h"

class KJob;

/** dolphinCommander runs the copies, moves and pastes it starts one after another, like OneCommander's queue of file operations. */
namespace JobQueue
{
/**
 * Lets @p job run after the jobs that were queued before it: a job queued while another one runs is suspended
 * until that one is done. Does nothing when the queue is turned off.
 */
DOLPHIN_EXPORT void add(KJob *job);
DOLPHIN_EXPORT void setEnabled(bool enabled);
DOLPHIN_EXPORT bool isEnabled();
/** How many jobs are running or waiting. */
DOLPHIN_EXPORT int count();
}

#endif
