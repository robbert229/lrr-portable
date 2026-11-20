// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_flic_size — G98-016's frame-size getters on a synthetic open flic.
 * FrontEnd.c, Pointer.c and Panel.c size Flic_Animate's destination from
 * them. The original's Flic_GetWidth was folded into AnimClone_IsLws, which
 * on 64-bit reads the flic's rambufferhandle instead of its width: 0 for a
 * streamed flic (the menu overlays drew nothing) and a pointer's low half
 * for one held in memory (the cursors). */
#include <stdio.h>
#include <string.h>

/* Flic_GetHeight links under its port name (flic_link_names.h). */
#define Flic_GetHeight object_field10_get
#include "gods98/flic_unit.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* What Flic_Setup leaves for a 176x232 flic, around the given file
 * buffer. */
static void make_flic(FLICSTRUCT* fsp, unsigned int flags, char* ram)
{
    memset(fsp, 0, sizeof(*fsp));
    fsp->userflags = flags;
    fsp->fsHeader.width = 176;
    fsp->fsHeader.height = 232;
    fsp->fsXsize = fsp->fsHeader.width;
    fsp->fsYsize = fsp->fsHeader.height;
    fsp->rambufferhandle = ram;
}

int main(void)
{
    static FLICSTRUCT fsp;
    static char ram[64];

    /* A streamed flic, as FrontEnd.c's overlays open them (flags 0). */
    make_flic(&fsp, 0, NULL);
    CHECK(Flic_GetWidth(&fsp) == 176, "streamed flic's width");
    CHECK(Flic_GetHeight(&fsp) == 232, "streamed flic's height");

    /* A flic held in memory and looping, as Pointer.c opens them. */
    make_flic(&fsp, FLICMEMORY | FLICLOOPINGON, ram);
    CHECK(Flic_GetWidth(&fsp) == 176, "in-memory flic's width");
    CHECK(Flic_GetHeight(&fsp) == 232, "in-memory flic's height");

    if (s_failures == 0) printf("flic_size: ok\n");
    return s_failures != 0;
}
