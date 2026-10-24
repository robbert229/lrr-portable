// SPDX-License-Identifier: LGPL-3.0-only
// Surface layer leaves: the RGB channel packer (finding 23/431). Draw
// helpers need a real framebuffer and stay under the platform suites.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/surface.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

int main(void)
{
    /* RGBChannelsPack (FUN_0047db90): byte triples pack little-endian —
     * first argument in the low byte, matching the palette scan. */
    CHECK(rgb_channels_pack(0x11, 0x22, 0x33) == 0x00332211u,
          "channels land low-to-high");
    CHECK(rgb_channels_pack(0, 0, 0) == 0u, "black packs zero");
    CHECK(rgb_channels_pack(255, 255, 255) == 0x00FFFFFFu,
          "white leaves the top byte clear");
    CHECK(rgb_channels_pack(0xFF, 0x00, 0xFF) == 0x00FF00FFu,
          "magenta keeps byte one empty");

    if (s_failures != 0) {
        fprintf(stderr, "surface_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("surface_test: PASS\n");
    return 0;
}
