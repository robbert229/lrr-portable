// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/platform.h"
#include "engine.hpp"
#include "frameloop.hpp"
#include "input.hpp"
#include "resource.hpp"
#include "surface.hpp"
#include "texture.hpp"

#define WINDOW_W 640
#define WINDOW_H 480
#define FRAME_MS 33 /* ~30 fps */

static const char* env_get(const char* name, const char* fallback)
{
    const char* v = getenv(name);
    return (v != NULL && v[0] != '\0') ? v : fallback;
}

int LegorrrRun(void)
{
    const char* wad_dir = env_get("LEGORRR_WAD_DIR", ".");
    const char* autostop = env_get("LEGORRR_AUTOSTOP_FRAMES", "0");
    long frame_cap = strtol(autostop, NULL, 10);
    if (frame_cap < 0) {
        frame_cap = 0;
    }

    FILE* log = fopen("legoRR-stub.log", "a");
    if (log == NULL) {
        log = stderr;
    }

    int mounted = resource_mount(wad_dir, "LegoRR");
    fprintf(log, "frameloop: mounted %d wad(s) from %s\n", mounted, wad_dir);
    if (mounted == 0) {
        if (log != stderr) {
            fclose(log);
        }
        return 0;
    }

    EngineState state = {};
    InputState  input = {};
    input_reset(&input);
    EngineContext ctx = {
        ENGINE_APP_NORMAL, 0, 0, 0, 1, &state, &input, 0.0f, 0,
    };

    PlatformWindow* win = platform_create_window("lrr-portable (frame loop)",
                                                 WINDOW_W, WINDOW_H);
    if (win == NULL) {
        fprintf(log, "frameloop: no platform window (headless?)\n");
        if (log != stderr) {
            fclose(log);
        }
        return 1;
    }

    DisplaySurface surf = { 0, 0, 0, 0, 0 };
    int rc = surface_create(&surf, platform_window_hwnd(win),
                            WINDOW_W, WINDOW_H);
    if (rc != 0) {
        fprintf(log, "frameloop: surface_create rc=%d\n", rc);
        platform_destroy_window(win);
        if (log != stderr) {
            fclose(log);
        }
        return 2;
    }

    /* Real corpus textures through the merged texture loader. */
    TextureSurface* rock = texture_get_or_load(NULL,
                                               "World\\WorldTextures\\ROCKROOF.BMP");
    TextureSurface* ui = texture_get_or_load(NULL,
                                             "Interface\\Buttons\\info_path.bmp");
    if (rock == NULL || ui == NULL) {
        fprintf(log, "frameloop: texture load failed (rock=%p ui=%p)\n",
                (void*)rock, (void*)ui);
    }

    const float dt = 1.0f / 30.0f;
    /* Interactive proof: the arrow keys pan the blitted texture. */
    int rock_x = 24, rock_y = 24;

    long frames = 0;
    int quit = 0;
    const char* stop = "closed";
    while (!quit) {
        /* 1. Device capture for this frame (the finding-05 "writer" half):
         *    platform events -> canonical codes -> input tables. */
        PlatformEvent evbuf[32];
        int nev = platform_pump_events(win, evbuf, 32);
        input_capture_events(&input, evbuf, nev);
        for (int i = 0; i < nev; ++i) {
            if (evbuf[i].kind == PLATFORM_EVENT_QUIT
                || (evbuf[i].kind == PLATFORM_EVENT_KEYDOWN
                    && evbuf[i].code == PLATFORM_KEY_ESC)) {
                quit = 1;
                stop = "quit/ESC";
                break;
            }
        }
        if (quit) {
            break;
        }
        if (frame_cap > 0 && frames >= frame_cap) {
            stop = "autostop";
            break;
        }

        /* 2. Fine-grained pan via key edges (cur vs prev). */
        if (input_key_edge(&input, PLATFORM_KEY_UP))    { rock_y -= 8; }
        if (input_key_edge(&input, PLATFORM_KEY_DOWN))  { rock_y += 8; }
        if (input_key_edge(&input, PLATFORM_KEY_LEFT))  { rock_x -= 8; }
        if (input_key_edge(&input, PLATFORM_KEY_RIGHT)) { rock_x += 8; }
        if (rock_x < 0) { rock_x = 0; }
        if (rock_y < 0) { rock_y = 0; }
        if (rock_x > WINDOW_W - 64) { rock_x = WINDOW_W - 64; }
        if (rock_y > WINDOW_H - 64) { rock_y = WINDOW_H - 64; }

        EngineTick(&ctx, dt);

        void* pixels = NULL;
        int   pitch = 0;
        if (surface_lock(&surf, &pixels, &pitch) == 0) {
            surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 16, 48);
            surface_draw_sweep(pixels, pitch, WINDOW_W, WINDOW_H, (int)frames);
            if (rock != NULL) {
                surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                     rock, rock_x, rock_y);
            }
            if (ui != NULL) {
                surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                     ui, rock_x + 64 + 8, rock_y);
            }
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)frames);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            surface_unlock(&surf, pixels);
        }
        Sleep((DWORD)FRAME_MS);
        ++frames;

        /* 3. Frame boundary: this frame's keys become prev; cur clears for
         *    the next capture (edge semantics, findings 04/05). */
        input_begin_frame(&input);
    }

    fprintf(log, "frameloop: %ld frames, gauged state=0x%08x, tickers=%d, "
            "exit=%s\n", frames, ctx.state, ctx.tickers_ran, stop);

    EngineShutdown(&ctx);
    texture_cache_clear();
    surface_destroy(&surf);
    platform_destroy_window(win);
    if (log != stderr) {
        fclose(log);
    }
    return 0;
}