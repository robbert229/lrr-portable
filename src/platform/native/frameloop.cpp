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
#include "gods98/frameloop.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>

struct PlatformWindow {
    SDL_Window*  sdl;
    SDL_Surface* window_surface; /* cached SDL_GetWindowSurface result */
    int          w;
    int          h;
};

static PlatformWindow* s_active_window;
static PlatformRenderSurfaceOps s_platform_render_surface_ops;
void* g_platform_cursor_window_handle;
int g_platform_cursor_width;
int g_platform_cursor_height;

void platform_render_surface_ops_set(const PlatformRenderSurfaceOps* ops)
{
    if (ops != NULL) s_platform_render_surface_ops = *ops;
    else memset(&s_platform_render_surface_ops, 0,
                sizeof(s_platform_render_surface_ops));
}

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
    s_active_window = pw;
    g_platform_cursor_window_handle = (void*)win;
    g_platform_cursor_width = width;
    g_platform_cursor_height = height;
    window_runtime_state_initialize((void*)win);
    /* The game draws its own bitmap pointer into the framebuffer. */
    SDL_HideCursor();
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
    SDL_ShowCursor();
    if (s_active_window == pw) {
        s_active_window = NULL;
        g_platform_cursor_window_handle = NULL;
        g_platform_cursor_width = 0;
        g_platform_cursor_height = 0;
        window_runtime_state_initialize(NULL);
    }
    pw->sdl = NULL;
    free(pw);
}

void platform_set_window_title(const char* title)
{
    if (s_active_window != NULL && s_active_window->sdl != NULL
        && title != NULL) {
        SDL_SetWindowTitle(s_active_window->sdl, title);
    }
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

int platform_cursor_position_get(int* x, int* y)
{
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    if (s_active_window == NULL || s_active_window->sdl == NULL ||
        x == NULL || y == NULL) {
        return 0;
    }
    SDL_GetMouseState(&mouse_x, &mouse_y);
    *x = (int)mouse_x;
    *y = (int)mouse_y;
    return 1;
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
            /* held is the level state of any button at this moment; the
             * press edge stays on `button` so click detection is unchanged */
            ev->held = (e.motion.state != 0) ? 1 : 0;
            ++n;
        } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            ev->kind = PLATFORM_EVENT_MOUSE;
            ev->mx = e.button.x;
            ev->my = e.button.y;
            ev->button = 1; /* left/any press edge; latched in the input */
            ev->held = 1;
            ++n;
        } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            /* release emits no motion if the pointer stands still, so the
             * held level must drop here or drags would phantom on */
            ev->kind = PLATFORM_EVENT_MOUSE;
            ev->mx = e.button.x;
            ev->my = e.button.y;
            ev->button = 0;
            ev->held = 0;
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
    if (s_platform_render_surface_ops.fullscreen_get != NULL &&
        s_platform_render_surface_ops.fullscreen_get(
            s_platform_render_surface_ops.context)) {
        if (s_platform_render_surface_ops.surface_flip != NULL) {
            s_platform_render_surface_ops.surface_flip(
                s_platform_render_surface_ops.context, pw, 0, 1);
        }
        return;
    }
    if (s_platform_render_surface_ops.windowed_blit != NULL) {
        s_platform_render_surface_ops.windowed_blit(
            s_platform_render_surface_ops.context, pw, pixels, pitch,
            width, height);
        return;
    }
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

int platform_audio_queued_milliseconds(PlatformAudio* a)
{
    int queued = platform_audio_queued(a);
    if (queued <= 0) return 0;
    return (int)(((double)queued * 1000.0) / (44100.0 * 4.0));
}

void platform_audio_stop(PlatformAudio* a)
{
    if (a == NULL || a->stream == NULL) {
        return;
    }
    platform_audio_loop(a, NULL, 0, 0);
    SDL_ClearAudioStream(a->stream);
}

static int platform_cd_audio_unavailable(int* error_code)
{
    if (error_code != NULL) *error_code = 1;
    return 0;
}

int platform_cd_audio_open(int* error_code)
{
    return platform_cd_audio_unavailable(error_code);
}

int platform_cd_audio_set_tmsf(int* error_code)
{
    return platform_cd_audio_unavailable(error_code);
}

int platform_cd_audio_play(int one_based_track, int* error_code)
{
    (void)one_based_track;
    return platform_cd_audio_unavailable(error_code);
}

int platform_cd_audio_stop(int* error_code)
{
    return platform_cd_audio_unavailable(error_code);
}

int platform_cd_audio_close(int* error_code)
{
    return platform_cd_audio_unavailable(error_code);
}

int platform_cd_audio_status(char* mode, int mode_capacity,
                             int* current_track, int* error_code)
{
    if (mode != NULL && mode_capacity > 0) mode[0] = '\0';
    if (current_track != NULL) *current_track = 0;
    return platform_cd_audio_unavailable(error_code);
}

int platform_cd_audio_error_text(int error_code, char* output,
                                 int output_capacity)
{
    static const char message[] = "CD audio is unavailable on this backend";
    int i;
    (void)error_code;
    if (output == NULL || output_capacity <= 0) return 0;
    for (i = 0; i + 1 < output_capacity && message[i] != '\0'; ++i)
        output[i] = message[i];
    output[i] = '\0';
    return 1;
}

uint32_t platform_time_milliseconds(void)
{
    return (uint32_t)SDL_GetTicks();
}
