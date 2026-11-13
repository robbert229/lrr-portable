// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail (ADR 0006 decision 4) of the movie unit (G98-001,
 * finding 618), native only. The original's code is movie_unit.cpp, which
 * plays movies through DirectShow and is empty in the native port. The
 * native port has no DirectShow, so these entry points load no movie:
 * movie_load returns NULL, and its callers (Lego.c's Lego_PlayMovie_old,
 * Reward.c, FrontEnd.c) skip the playback, as they did with the port's
 * earlier hook-table model. Native movie playback through movie.cpp's
 * FFmpeg reader is a port gap (the `port` label). */
#include "movie_unit.h"

#include <stddef.h>

#if defined(LEGORRR_BACKEND_MINIWIN)

extern "C" {

struct G98CMovie* movie_load(const char* fName)
{
    (void)fName;
    return NULL;
}

void movie_get_size(struct G98CMovie* mov, unsigned int* width,
                    unsigned int* height)
{
    (void)mov;
    *width = 0;
    *height = 0;
}

unsigned int movie_get_duration(struct G98CMovie* mov)
{
    (void)mov;
    return 0;
}

int movie_update(struct G98CMovie* mov, float speed,
                 struct tagRECT* destRect)
{
    (void)mov;
    (void)speed;
    (void)destRect;
    return 0;
}

void movie_free(struct G98CMovie* mov)
{
    (void)mov;
}

} /* extern "C" */

#endif /* LEGORRR_BACKEND_MINIWIN */
