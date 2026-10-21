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
            } else if (kc == SDLK_RETURN || kc == SDLK_KP_ENTER) {
                code = PLATFORM_KEY_ENTER;
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
            ev->button = 0;
            ++n;
        } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            ev->kind = PLATFORM_EVENT_MOUSE;
            ev->mx = e.button.x;
            ev->my = e.button.y;
            ev->button = 1; /* left/any press edge; latched in the input */
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

/* ------------------------------------------------------------------ *
 *  Audio — SDL3 audio-stream device (mono/stereo 16-bit PCM submit).
 * ------------------------------------------------------------------ */

struct PlatformAudio {
    SDL_AudioStream*  stream;
    SDL_AudioDeviceID dev;
    int               ok;
    int16_t*          loop;      /* owned loop buffer (interleaved L,R) */
    int               loop_frames;
    int               loop_rate;
    int               loop_bytes;
};

PlatformAudio* platform_audio_open(void)
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "platform: SDL_InitSubSystem(AUDIO): %s\n",
                SDL_GetError());
        return NULL;
    }
    SDL_AudioSpec spec;
    memset(&spec, 0, sizeof(spec));
    spec.format   = SDL_AUDIO_S16;
    spec.channels = 2;   /* output; source is per-play mono or stereo */
    spec.freq     = 44100;

    PlatformAudio* a = (PlatformAudio*)calloc(1, sizeof(PlatformAudio));
    if (a == NULL) {
        return NULL;
    }
    a->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                          &spec, NULL, NULL);
    if (a->stream == NULL) {
        fprintf(stderr, "platform: SDL_OpenAudioDeviceStream: %s\n",
                SDL_GetError());
        free(a);
        return NULL;
    }
    /* Device streams open *paused* — without this nothing would ever play. */
    SDL_ResumeAudioStreamDevice(a->stream);
    a->ok = 1;
    SDL_AudioDeviceID dev = SDL_GetAudioStreamDevice(a->stream);
    fprintf(stderr, "platform: audio stream live on device %u\n",
            (unsigned)dev);
    return a;
}

void platform_audio_close(PlatformAudio* a)
{
    if (a == NULL) {
        return;
    }
    if (a->stream != NULL) {
        SDL_DestroyAudioStream(a->stream);
    }
    if (a->dev != 0) {
        SDL_CloseAudioDevice(a->dev);
    }
    free(a->loop);
    a->stream = NULL;
    a->loop = NULL;
    a->loop_frames = 0;
    a->ok = 0;
    free(a);
}

/* Set the source format of the stream for the current submission. */
static int a_set_src(PlatformAudio* a, int channels, int rate)
{
    SDL_AudioSpec src;
    memset(&src, 0, sizeof(src));
    src.format   = SDL_AUDIO_S16;
    src.channels = channels;
    src.freq     = rate;
    return SDL_SetAudioStreamFormat(a->stream, &src, NULL) ? 0 : -1;
}

int platform_audio_play(PlatformAudio* a, const int16_t* pcm, int frames,
                        int sample_rate)
{
    if (a == NULL || a->stream == NULL || pcm == NULL || frames <= 0
        || sample_rate <= 0) {
        return -1;
    }
    if (a_set_src(a, 1, sample_rate) != 0) {
        return -1;
    }
    return SDL_PutAudioStreamData(a->stream, pcm, frames * 2) ? 0 : -1;
}

int platform_audio_play_stereo(PlatformAudio* a, const int16_t* lr,
                                int frames, int sample_rate)
{
    if (a == NULL || a->stream == NULL || lr == NULL || frames <= 0
        || sample_rate <= 0) {
        return -1;
    }
    if (a_set_src(a, 2, sample_rate) != 0) {
        return -1;
    }
    return SDL_PutAudioStreamData(a->stream, lr, frames * 4) ? 0 : -1;
}

int platform_audio_loop(PlatformAudio* a, const int16_t* lr,
                        int frames, int sample_rate)
{
    if (a == NULL) {
        return -1;
    }
    free(a->loop);
    a->loop = NULL;
    a->loop_frames = 0;
    a->loop_rate = 0;
    a->loop_bytes = 0;
    if (lr == NULL || frames <= 0 || sample_rate <= 0) {
        return 0; /* stop the loop */
    }
    int16_t* copy = (int16_t*)calloc((size_t)frames * 2, 2);
    if (copy == NULL) {
        return -1;
    }
    memcpy(copy, lr, (size_t)frames * 4);
    a->loop = copy;
    a->loop_frames = frames;
    a->loop_rate = sample_rate;
    a->loop_bytes = frames * 4;
    return a_set_src(a, 2, sample_rate); /* seed format for refills */
}

void platform_audio_refill(PlatformAudio* a)
{
    if (a == NULL || a->stream == NULL || a->loop == NULL || a->loop_bytes <= 0) {
        return;
    }
    /* Keep the loop ahead of the drain: re-push once the queue drops below
     * one full loop. */
    if (SDL_GetAudioStreamQueued(a->stream) < a->loop_bytes / 2) {
        a_set_src(a, 2, a->loop_rate);
        SDL_PutAudioStreamData(a->stream, a->loop, a->loop_bytes);
    }
}

int platform_audio_set_gain(PlatformAudio* a, float gain)
{
    if (a == NULL || a->stream == NULL) {
        return -1;
    }
    if (gain < 0.0f) {
        gain = 0.0f;
    }
    if (gain > 1.0f) {
        gain = 1.0f;
    }
    return SDL_SetAudioStreamGain(a->stream, gain) ? 0 : -1;
}

float platform_audio_get_gain(PlatformAudio* a)
{
    if (a == NULL || a->stream == NULL) {
        return 0.0f;
    }
    return SDL_GetAudioStreamGain(a->stream);
}

int platform_audio_queued(PlatformAudio* a)
{
    if (a == NULL || a->stream == NULL) {
        return 0;
    }
    return SDL_GetAudioStreamQueued(a->stream);
}

void platform_audio_stop(PlatformAudio* a)
{
    if (a == NULL || a->stream == NULL) {
        return;
    }
    platform_audio_loop(a, NULL, 0, 0);
    SDL_ClearAudioStream(a->stream);
}