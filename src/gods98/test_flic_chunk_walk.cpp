// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_flic_chunk_walk — Flic_Memory's chunk walk on a synthetic in-memory
 * flic, as Pointer.c animates its cursors (FLICMEMORY | FLICLOOPINGON).
 * Each call draws one frame and leaves pointerposition at the next. The
 * port used to step on past the frame and add the next chunk's size too,
 * which skipped every other frame and, before the loop reset, read chunk
 * sizes from beyond the buffer: the crash in Flic_FindChunk under
 * Pointer_DrawPointer. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/flic_unit.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* FRAME_COUNT frames and the ring frame after them, as every flic in the
 * game's data has (header.frames + 1 frame chunks). */
enum { FRAME_COUNT = 4, CHUNK_COUNT = FRAME_COUNT + 1, FRAME_SIZE = 16,
       DATA_SIZE = CHUNK_COUNT * FRAME_SIZE };

/* What Flic_Setup leaves for such a flic of empty frames (each a bare
 * 16-byte 0xf1fa header with no subchunks), read from past the file's
 * 128-byte header into a buffer of the file's size. */
static void make_flic(FLICSTRUCT* fsp, unsigned int flags, char* ram)
{
    int i;

    memset(fsp, 0, sizeof(*fsp));
    fsp->userflags = flags;
    fsp->fsHeader.size = 128 + DATA_SIZE;
    fsp->fsHeader.frames = FRAME_COUNT;
    fsp->rambufferhandle = ram;
    /* The uninitialised tail Flic_Setup's malloc leaves past the data: a
     * huge chunk size there must never be followed. */
    memset(ram, 0x7f, fsp->fsHeader.size);
    for (i = 0; i < CHUNK_COUNT; i++) {
        char* frame = ram + i * FRAME_SIZE;
        int size = FRAME_SIZE;
        unsigned short type = 0xf1fa;
        memset(frame, 0, FRAME_SIZE);
        memcpy(frame, &size, sizeof(size));
        memcpy(frame + 4, &type, sizeof(type));
    }
}

int main(void)
{
    static FLICSTRUCT fsp;
    char* ram = (char*)malloc(128 + DATA_SIZE);
    int call;
    int ok;
    int pos;
    int frame;

    /* Looping, as Pointer.c opens its cursors: one frame per call. The ring
     * frame takes the last frame back to the first, so after it the walk
     * resumes at the second (Flic_Memory notes ringframe after drawing the
     * first). */
    make_flic(&fsp, FLICMEMORY | FLICLOOPINGON, ram);
    pos = 0;
    frame = 0;
    for (call = 0; call < 4 * CHUNK_COUNT; call++) {
        ok = Flic_Memory(&fsp);
        pos += FRAME_SIZE;
        frame++;
        if (frame == FRAME_COUNT + 1) {
            frame = 1;
            pos = FRAME_SIZE;
        }
        CHECK(ok == 0, "a looping flic never reports its end");
        CHECK(fsp.pointerposition == pos, "one frame per call, looping");
        CHECK(fsp.currentframe == frame, "frame counter, looping");
    }

    /* Not looping, and animated on past its last frame: the walk stays at
     * the end of the data instead of reading beyond it. */
    make_flic(&fsp, FLICMEMORY, ram);
    for (call = 0; call < CHUNK_COUNT; call++) {
        ok = Flic_Memory(&fsp);
        CHECK(fsp.pointerposition == (call + 1) * FRAME_SIZE,
              "one frame per call, not looping");
    }
    CHECK(ok == 1, "the last frame reports the end");
    for (call = 0; call < 4; call++) {
        Flic_Memory(&fsp);
        CHECK(fsp.pointerposition == DATA_SIZE,
              "past the end, the walk stays at the end of the data");
    }

    free(ram);
    if (s_failures == 0) printf("flic_chunk_walk: ok\n");
    return s_failures != 0;
}
