// SPDX-License-Identifier: LGPL-3.0-or-later
// The keyboard substitution's table (Input.c, `// [library:input]`): SDL
// scancodes land in the DirectInput key map at their DIK_* codes.
// Synthetic only: no SDL subsystem and no game data.
#include "input_sdl_keys.h"

#include <stdio.h>

static int failures;

#define CHECK_DIK(scancode, dik)                                            \
    do {                                                                    \
        unsigned got = input_sdl_scancode_dik(scancode);                    \
        if (got != (dik)) {                                                 \
            fprintf(stderr, "%s:%d: %s maps to 0x%02x, expected 0x%02x\n",  \
                    __FILE__, __LINE__, #scancode, got, (unsigned)(dik));   \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

int main()
{
    // Letters follow the keyboard rows: DIK_Q 0x10, DIK_A 0x1e, DIK_Z 0x2c.
    CHECK_DIK(SDL_SCANCODE_A, 0x1e);
    CHECK_DIK(SDL_SCANCODE_M, 0x32);
    CHECK_DIK(SDL_SCANCODE_Q, 0x10);
    CHECK_DIK(SDL_SCANCODE_Z, 0x2c);
    CHECK_DIK(SDL_SCANCODE_1, 0x02);
    CHECK_DIK(SDL_SCANCODE_9, 0x0a);
    CHECK_DIK(SDL_SCANCODE_0, 0x0b);
    CHECK_DIK(SDL_SCANCODE_ESCAPE, 0x01);
    CHECK_DIK(SDL_SCANCODE_RETURN, 0x1c);
    CHECK_DIK(SDL_SCANCODE_SPACE, 0x39);
    CHECK_DIK(SDL_SCANCODE_F1, 0x3b);
    CHECK_DIK(SDL_SCANCODE_F10, 0x44);
    CHECK_DIK(SDL_SCANCODE_F12, 0x58);
    CHECK_DIK(SDL_SCANCODE_LSHIFT, 0x2a);
    CHECK_DIK(SDL_SCANCODE_RCTRL, 0x9d);
    CHECK_DIK(SDL_SCANCODE_KP_ENTER, 0x9c);
    CHECK_DIK(SDL_SCANCODE_UP, 0xc8);
    CHECK_DIK(SDL_SCANCODE_DOWN, 0xd0);
    CHECK_DIK(SDL_SCANCODE_LEFT, 0xcb);
    CHECK_DIK(SDL_SCANCODE_RIGHT, 0xcd);
    CHECK_DIK(SDL_SCANCODE_DELETE, 0xd3);
    CHECK_DIK(SDL_SCANCODE_UNKNOWN, 0x00);
    CHECK_DIK(SDL_SCANCODE_AC_BACK, 0x00);

    // No two keys share a DIK code, except the ISO key beside Enter, which
    // DirectInput reports as DIK_BACKSLASH.
    int owner[256];
    for (int i = 0; i < 256; ++i) {
        owner[i] = -1;
    }
    for (int s = 0; s < SDL_SCANCODE_COUNT; ++s) {
        if (s == SDL_SCANCODE_NONUSHASH) {
            continue;
        }
        unsigned dik = input_sdl_scancode_dik((SDL_Scancode) s);
        if (!dik) {
            continue;
        }
        if (owner[dik] != -1) {
            fprintf(stderr, "scancodes %d and %d both map to 0x%02x\n", owner[dik], s, dik);
            ++failures;
        }
        owner[dik] = s;
    }

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("input keys: ok\n");
    return 0;
}
