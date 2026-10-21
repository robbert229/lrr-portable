// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native platform glue — miniwin (SDL3) window, events, presentation.
 *
 * miniwin reimplements the DX-era surface (DirectDraw etc.) over SDL3 but
 * deliberately leaves three things to the app: creating the SDL window (its
 * SetCooperativeLevel casts the HWND straight to SDL_Window*), pumping SDL
 * events, and presenting the framebuffer (miniwin's surface Flip is a stub).
 * That glue lives here, on the native side of the seam (docs/adr/0004).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/platform.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>

struct PlatformWindow {
    SDL_Window*  sdl;
    SDL_Surface* window_surface; /* cached SDL_GetWindowSurface result */
    int          w;
    int          h;
};

PlatformWindow* platform_create_window(const char* title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "platform: SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
    SDL_Window* win = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (win == NULL) {
        fprintf(stderr, "platform: SDL_CreateWindowWithProperties failed: %s\n",
                SDL_GetError());
        return NULL;
    }

    PlatformWindow* pw = (PlatformWindow*)calloc(1, sizeof(PlatformWindow));
    if (pw == NULL) {
        SDL_DestroyWindow(win);
        return NULL;
    }
    pw->sdl = win;
    pw->w = width;
    pw->h = height;
    pw->window_surface = SDL_GetWindowSurface(win);
    return pw;
}

void platform_destroy_window(PlatformWindow* pw)
{
    if (pw == NULL) {
        return;
    }
    if (pw->sdl != NULL) {
        SDL_DestroyWindow(pw->sdl);
    }
    pw->sdl = NULL;
    free(pw);
}

void* platform_window_hwnd(PlatformWindow* pw)
{
    return pw != NULL ? (void*)pw->sdl : NULL;
}

int platform_window_width(PlatformWindow* pw)
{
    return pw != NULL ? pw->w : 0;
}

int platform_window_height(PlatformWindow* pw)
{
    return pw != NULL ? pw->h : 0;
}

int platform_pump_events(PlatformWindow* pw, PlatformEvent* out, int cap)
{
    if (pw == NULL || out == NULL || cap <= 0) {
        return 0;
    }
    SDL_Event e;
    int n = 0;
    while (SDL_PollEvent(&e)) {
        if (n >= cap) {
            break;
        }
        PlatformEvent* ev = &out[n];
        ev->kind = PLATFORM_EVENT_NONE;
        ev->code = 0;
        ev->mx = 0;
        ev->my = 0;
        if (e.type == SDL_EVENT_QUIT
            || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            ev->kind = PLATFORM_EVENT_QUIT;
            ++n;
        } else if (e.type == SDL_EVENT_KEY_DOWN
                   || e.type == SDL_EVENT_KEY_UP) {
            /* Printable ASCII passes through (canonical vocabulary); the
             * navigation controls map to the stable codes in platform.h. */
            SDL_Keycode kc = e.key.key;
            int code = 0;
            if (kc >= 0x20 && kc <= 0x7e) {
                code = (int)kc;
            } else if (kc == SDLK_UP) {
                code = PLATFORM_KEY_UP;
            } else if (kc == SDLK_DOWN) {
                code = PLATFORM_KEY_DOWN;
            } else if (kc == SDLK_LEFT) {
                code = PLATFORM_KEY_LEFT;
            } else if (kc == SDLK_RIGHT) {
                code = PLATFORM_KEY_RIGHT;
            } else if (kc == SDLK_ESCAPE) {
                code = PLATFORM_KEY_ESC;
            }
            if (code != 0) {
                ev->kind = (e.type == SDL_EVENT_KEY_DOWN)
                               ? PLATFORM_EVENT_KEYDOWN
                               : PLATFORM_EVENT_KEYUP;
                ev->code = code;
                ++n;
            }
        } else if (e.type == SDL_EVENT_MOUSE_MOTION) {
            ev->kind = PLATFORM_EVENT_MOUSE;
            ev->mx = e.motion.x;
            ev->my = e.motion.y;
            ++n;
        }
    }
    return n;
}

int platform_poll_events(PlatformWindow* pw)
{
    PlatformEvent evb[32];
    int n = platform_pump_events(pw, evb, 32);
    for (int i = 0; i < n; ++i) {
        if (evb[i].kind == PLATFORM_EVENT_QUIT
            || (evb[i].kind == PLATFORM_EVENT_KEYDOWN
                && evb[i].code == PLATFORM_KEY_ESC)) {
            return 1;
        }
    }
    return 0;
}

void platform_present_pixels(PlatformWindow* pw,
                             const void* pixels, int pitch,
                             int width, int height)
{
    if (pw == NULL || pw->sdl == NULL || pw->window_surface == NULL) {
        return;
    }
    /* Present the frame after the miniwin surface Unlock ran? No: we are
     * called while the miniwin surface is still Locked (SDL_LockSurface is
     * reentrant/refcounted), so wrapping the buffer now is safe. */
    SDL_Surface* tmp = SDL_CreateSurfaceFrom(width, height,
                                             SDL_PIXELFORMAT_RGBA32,
                                             (void*)pixels, pitch);
    if (tmp == NULL) {
        return;
    }
    SDL_BlitSurface(tmp, NULL, pw->window_surface, NULL);
    SDL_DestroySurface(tmp);
    SDL_UpdateWindowSurface(pw->sdl);
}

void platform_screenshot(PlatformWindow* pw, const void* pixels, int pitch,
                         int width, int height, int frame)
{
    (void)pw;
    const char* path = getenv("LEGORRR_SCREENSHOT");
    if (path == NULL || pixels == NULL) {
        return;
    }
    /* PPM P6, R,G,B from RGBA32 (byte order R,G,B,A on LE). */
    FILE* f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "platform: cannot write screenshot %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    for (int y = 0; y < height; ++y) {
        const unsigned char* row = (const unsigned char*)pixels + y * pitch;
        for (int x = 0; x < width; ++x) {
            fputc(row[x * 4 + 0], f); /* R */
            fputc(row[x * 4 + 1], f); /* G */
            fputc(row[x * 4 + 2], f); /* B */
        }
    }
    fclose(f);
    fprintf(stderr, "platform: screenshot %s (frame %d) %s\n", path, frame,
            "saved");
}