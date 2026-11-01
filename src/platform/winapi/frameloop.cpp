// SPDX-License-Identifier: LGPL-3.0-or-later
/* winapi platform glue — Win32 window/message APIs with presentation calls
 * delegated through the installable rendering interface.
 */
#include "platform/platform.h"
#include "gods98/frameloop.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>

struct PlatformWindow {
    HWND hwnd;
    int width;
    int height;
};

static const char s_platform_window_class[] = "LegoRRPortableWindow";
static PlatformWindow* s_active_window;

namespace unported_ledger_stubs {
ptrdiff_t ledger_stub_004784c0_GameWindowProcedure(
    void* window, uint32_t message, size_t wparam, ptrdiff_t lparam);
int ledger_stub_00478010_WindowMessageDispatchFilter(uint32_t message);
}

static ptrdiff_t platform_default_window_proc(void* context, void* window,
    uint32_t message, size_t wparam, ptrdiff_t lparam)
{
    (void)context;
    return (ptrdiff_t)DefWindowProcA((HWND)window, (UINT)message,
                                   (WPARAM)wparam, (LPARAM)lparam);
}

static int platform_device_caps_query(void* context, void* window, int index)
{
    HDC dc;
    int value;
    (void)context;
    dc = GetDC((HWND)window);
    if (dc == NULL) return 0;
    value = GetDeviceCaps(dc, index);
    ReleaseDC((HWND)window, dc);
    return value;
}

static int platform_adjust_window_rect(void* context, WindowPortRect* port_rect,
                                       int32_t style, int has_menu)
{
    RECT rect;
    (void)context;
    if (port_rect == NULL) return 0;
    rect.left = port_rect->left;
    rect.top = port_rect->top;
    rect.right = port_rect->right;
    rect.bottom = port_rect->bottom;
    if (!AdjustWindowRect(&rect, (DWORD)style, has_menu != 0)) return 0;
    port_rect->left = rect.left;
    port_rect->top = rect.top;
    port_rect->right = rect.right;
    port_rect->bottom = rect.bottom;
    return 1;
}

static void platform_mouse_capture(void* context, void* window)
{
    (void)context;
    SetCapture((HWND)window);
}

static void platform_mouse_release(void* context)
{
    (void)context;
    ReleaseCapture();
}

static LRESULT CALLBACK platform_window_proc(HWND hwnd, UINT message,
                                              WPARAM wparam, LPARAM lparam)
{
    if (s_active_window != NULL && s_active_window->hwnd == hwnd &&
        message == WM_SIZE) {
        s_active_window->width = (int)(lparam & 0xffff);
        s_active_window->height = (int)((lparam >> 16) & 0xffff);
    }
    return (LRESULT)unported_ledger_stubs::ledger_stub_004784c0_GameWindowProcedure(
        (void*)hwnd, (uint32_t)message, (size_t)wparam,
        (ptrdiff_t)lparam);
}

static PlatformRenderSurfaceOps s_platform_render_surface_ops;

void platform_render_surface_ops_set(const PlatformRenderSurfaceOps* ops)
{
    if (ops != NULL) s_platform_render_surface_ops = *ops;
    else memset(&s_platform_render_surface_ops, 0,
                sizeof(s_platform_render_surface_ops));
}

// FUNCTION: LEGORR 0x004783f0
PlatformWindow* platform_create_window(const char* title, int width, int height)
{
    WNDCLASSA wc;
    ATOM class_atom;
    HWND hwnd;
    PlatformWindow* win;
    WindowRuntimeOps window_ops;
    HINSTANCE instance = GetModuleHandleA(NULL);
    memset(&window_ops, 0, sizeof(window_ops));
    window_ops.device_caps_query = platform_device_caps_query;
    window_ops.adjust_window_rect = platform_adjust_window_rect;
    window_ops.mouse_capture = platform_mouse_capture;
    window_ops.mouse_release = platform_mouse_release;
    window_runtime_ops_install_defaults(&window_ops);
    memset(&wc, 0, sizeof(wc));
    wc.style = 8;
    wc.lpfnWndProc = platform_window_proc;
    wc.hInstance = instance;
    wc.lpszClassName = s_platform_window_class;
    class_atom = RegisterClassA(&wc);
    if (class_atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return NULL;
    }
    window_runtime_default_proc_set(platform_default_window_proc, NULL);
    if (title == NULL) title = "Lego Rock Raiders";
    hwnd = CreateWindowExA(0x00040000u, s_platform_window_class, title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        width > 0 ? width : 640, height > 0 ? height : 480,
        NULL, NULL, instance, NULL);
    if (hwnd == NULL) return NULL;
    win = (PlatformWindow*)calloc(1, sizeof(*win));
    if (win == NULL) {
        DestroyWindow(hwnd);
        return NULL;
    }
    win->hwnd = hwnd;
    win->width = width > 0 ? width : 640;
    win->height = height > 0 ? height : 480;
    s_active_window = win;
    window_runtime_state_initialize((void*)hwnd);
    SetFocus(hwnd);
    return win;
}

void platform_destroy_window(PlatformWindow* win)
{
    if (win == NULL) return;
    if (win->hwnd != NULL) DestroyWindow(win->hwnd);
    if (s_active_window == win) {
        s_active_window = NULL;
        window_runtime_state_initialize(NULL);
    }
    free(win);
}

void platform_set_window_title(const char* title)
{
    if (s_active_window != NULL && s_active_window->hwnd != NULL &&
        title != NULL) SetWindowTextA(s_active_window->hwnd, title);
}

void* platform_window_hwnd(PlatformWindow* win)
{
    return win != NULL ? (void*)win->hwnd : NULL;
}

int platform_window_width(PlatformWindow* win)
{
    return win != NULL ? win->width : 0;
}

int platform_window_height(PlatformWindow* win)
{
    return win != NULL ? win->height : 0;
}

int platform_window_cursor_position_get(PlatformWindow* win, int* x, int* y)
{
    RECT client_rect;
    POINT client_origin;
    POINT cursor;
    if (win == NULL || win->hwnd == NULL || x == NULL || y == NULL ||
        !GetClientRect(win->hwnd, &client_rect)) {
        return 0;
    }
    client_origin.x = 0;
    client_origin.y = 0;
    cursor.x = 0;
    cursor.y = 0;
    ClientToScreen(win->hwnd, &client_origin);
    GetCursorPos(&cursor);
    *x = cursor.x - client_origin.x;
    *y = cursor.y - client_origin.y;
    return 1;
}

int platform_poll_events(PlatformWindow* win)
{
    PlatformEvent events[32];
    int count = platform_pump_events(win, events, 32);
    int i;
    for (i = 0; i < count; ++i) {
        if (events[i].kind == PLATFORM_EVENT_QUIT ||
            (events[i].kind == PLATFORM_EVENT_KEYDOWN &&
             events[i].code == PLATFORM_KEY_ESC)) return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00478040
int platform_pump_events(PlatformWindow* win, PlatformEvent* out, int cap)
{
    MSG message;
    int count = 0;
    if (win == NULL || out == NULL || cap <= 0) return 0;
    while (count < cap && PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        PlatformEvent* event = &out[count];
        int accepted = unported_ledger_stubs::ledger_stub_00478010_WindowMessageDispatchFilter(
            (uint32_t)message.message);
        if (message.message == WM_QUIT) {
            event->kind = PLATFORM_EVENT_QUIT;
            event->code = event->mx = event->my = 0;
            event->button = event->held = 0;
            ++count;
            continue;
        }
        if (accepted) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        event->kind = PLATFORM_EVENT_NONE;
        event->code = event->mx = event->my = 0;
        event->button = event->held = 0;
        if (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN ||
            message.message == WM_KEYUP || message.message == WM_SYSKEYUP) {
            int code = 0;
            if (message.wParam >= 0x20 && message.wParam <= 0x7e)
                code = (int)message.wParam;
            else if (message.wParam == VK_UP) code = PLATFORM_KEY_UP;
            else if (message.wParam == VK_DOWN) code = PLATFORM_KEY_DOWN;
            else if (message.wParam == VK_LEFT) code = PLATFORM_KEY_LEFT;
            else if (message.wParam == VK_RIGHT) code = PLATFORM_KEY_RIGHT;
            else if (message.wParam == VK_ESCAPE) code = PLATFORM_KEY_ESC;
            else if (message.wParam == VK_RETURN) code = PLATFORM_KEY_ENTER;
            if (accepted && code != 0) {
                event->kind = (message.message == WM_KEYDOWN ||
                               message.message == WM_SYSKEYDOWN)
                    ? PLATFORM_EVENT_KEYDOWN : PLATFORM_EVENT_KEYUP;
                event->code = code;
            }
        }
        else if (message.message == WM_MOUSEMOVE ||
                 message.message == WM_LBUTTONDOWN ||
                 message.message == WM_LBUTTONUP ||
                 message.message == WM_RBUTTONDOWN ||
                 message.message == WM_RBUTTONUP) {
            event->kind = PLATFORM_EVENT_MOUSE;
            event->mx = (short)(message.lParam & 0xffff);
            event->my = (short)((message.lParam >> 16) & 0xffff);
            event->button = (message.message == WM_LBUTTONDOWN ||
                            message.message == WM_RBUTTONDOWN) ? 1 : 0;
            event->held = (message.wParam & (MK_LBUTTON | MK_RBUTTON)) != 0;
        }
        else if (message.message == WM_CLOSE) {
            event->kind = PLATFORM_EVENT_QUIT;
        }
        if (event->kind != PLATFORM_EVENT_NONE) ++count;
    }
    return count;
}

// FUNCTION: LEGORR 0x0047c8d0
void platform_present_pixels(PlatformWindow* win, const void* pixels, int pitch,
                             int width, int height)
{
    int fullscreen = 0;
    if (s_platform_render_surface_ops.fullscreen_get != NULL) {
        fullscreen = s_platform_render_surface_ops.fullscreen_get(
            s_platform_render_surface_ops.context);
    }
    if (fullscreen) {
        if (s_platform_render_surface_ops.surface_flip != NULL) {
            s_platform_render_surface_ops.surface_flip(
                s_platform_render_surface_ops.context, win, 0, 1);
        }
    }
    else if (s_platform_render_surface_ops.windowed_blit != NULL) {
        s_platform_render_surface_ops.windowed_blit(
            s_platform_render_surface_ops.context, win, pixels, pitch,
            width, height);
    }
}

void platform_screenshot(PlatformWindow* win, const void* pixels, int pitch,
                         int width, int height, int frame)
{
    (void)win; (void)pixels; (void)pitch; (void)width; (void)height;
    (void)frame;
}

typedef struct PlatformAudioSubmission {
    HWAVEOUT output;
    WAVEHDR header;
    DWORD bytes;
    DWORD sample_rate;
    WORD channels;
    int is_loop;
    struct PlatformAudioSubmission* next;
} PlatformAudioSubmission;

struct PlatformAudio {
    PlatformAudioSubmission* submissions;
    PlatformAudioSubmission* loop;
    float gain;
};

static DWORD platform_audio_volume(float gain)
{
    DWORD channel = (DWORD)(gain * 65535.0f);
    return channel | (channel << 16);
}

static void platform_audio_submission_destroy(PlatformAudioSubmission* item,
                                              int reset)
{
    if (item == NULL) return;
    if (item->output != NULL) {
        if (reset) waveOutReset(item->output);
        waveOutUnprepareHeader(item->output, &item->header,
                               sizeof(item->header));
        waveOutClose(item->output);
    }
    free(item->header.lpData);
    free(item);
}

static void platform_audio_reap(PlatformAudio* a)
{
    PlatformAudioSubmission** link;
    if (a == NULL) return;
    link = &a->submissions;
    while (*link != NULL) {
        PlatformAudioSubmission* item = *link;
        if (!item->is_loop && (item->header.dwFlags & WHDR_DONE) != 0) {
            *link = item->next;
            platform_audio_submission_destroy(item, 0);
        }
        else {
            link = &item->next;
        }
    }
}

static int platform_audio_submit(PlatformAudio* a, const int16_t* pcm,
                                 int frames, int sample_rate, int channels,
                                 int loop)
{
    PlatformAudioSubmission* item;
    WAVEFORMATEX format;
    DWORD bytes;
    MMRESULT result;

    platform_audio_reap(a);
    if (a == NULL || pcm == NULL || frames <= 0 || sample_rate <= 0 ||
        (channels != 1 && channels != 2) ||
        sample_rate > 0x3fffffff / (channels * 2) ||
        frames > 0x3fffffff / (channels * 2)) return -1;
    bytes = (DWORD)(frames * channels * 2);
    item = (PlatformAudioSubmission*)calloc(1, sizeof(*item));
    if (item == NULL) return -1;
    item->header.lpData = (LPSTR)malloc(bytes);
    if (item->header.lpData == NULL) {
        free(item);
        return -1;
    }
    memcpy(item->header.lpData, pcm, bytes);
    item->bytes = bytes;
    item->sample_rate = (DWORD)sample_rate;
    item->channels = (WORD)channels;
    item->is_loop = loop;

    memset(&format, 0, sizeof(format));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = (WORD)channels;
    format.nSamplesPerSec = (DWORD)sample_rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = (WORD)(channels * 2);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    result = waveOutOpen(&item->output, WAVE_MAPPER, &format, 0, 0,
                         CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        platform_audio_submission_destroy(item, 0);
        return -1;
    }
    waveOutSetVolume(item->output, platform_audio_volume(a->gain));
    item->header.dwBufferLength = bytes;
    if (loop) {
        item->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        item->header.dwLoops = 0xffffffffUL;
    }
    result = waveOutPrepareHeader(item->output, &item->header,
                                  sizeof(item->header));
    if (result != MMSYSERR_NOERROR) {
        platform_audio_submission_destroy(item, 0);
        return -1;
    }
    result = waveOutWrite(item->output, &item->header, sizeof(item->header));
    if (result != MMSYSERR_NOERROR) {
        platform_audio_submission_destroy(item, 1);
        return -1;
    }
    item->next = a->submissions;
    a->submissions = item;
    if (loop) a->loop = item;
    return 0;
}

PlatformAudio* platform_audio_open(void)
{
    PlatformAudio* a;
    if (waveOutGetNumDevs() == 0) return NULL;
    a = (PlatformAudio*)calloc(1, sizeof(*a));
    if (a != NULL) a->gain = 1.0f;
    return a;
}

void platform_audio_close(PlatformAudio* a)
{
    if (a == NULL) return;
    platform_audio_stop(a);
    free(a);
}

int platform_audio_play(PlatformAudio* a, const int16_t* pcm, int frames,
                        int sample_rate)
{
    return platform_audio_submit(a, pcm, frames, sample_rate, 1, 0);
}

int platform_audio_play_stereo(PlatformAudio* a, const int16_t* lr,
                               int frames, int sample_rate)
{
    return platform_audio_submit(a, lr, frames, sample_rate, 2, 0);
}

int platform_audio_loop(PlatformAudio* a, const int16_t* lr,
                        int frames, int sample_rate)
{
    if (a == NULL) return -1;
    if (a->loop != NULL) {
        PlatformAudioSubmission** link = &a->submissions;
        while (*link != NULL && *link != a->loop) link = &(*link)->next;
        if (*link != NULL) {
            PlatformAudioSubmission* old = *link;
            *link = old->next;
            a->loop = NULL;
            platform_audio_submission_destroy(old, 1);
        }
    }
    if (lr == NULL || frames <= 0 || sample_rate <= 0) return 0;
    return platform_audio_submit(a, lr, frames, sample_rate, 2, 1);
}

void platform_audio_refill(PlatformAudio* a)
{
    platform_audio_reap(a);
}

int platform_audio_set_gain(PlatformAudio* a, float gain)
{
    PlatformAudioSubmission* item;
    int result = 0;
    if (a == NULL) return -1;
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    a->gain = gain;
    for (item = a->submissions; item != NULL; item = item->next) {
        if (waveOutSetVolume(item->output, platform_audio_volume(gain)) !=
            MMSYSERR_NOERROR) result = -1;
    }
    return result;
}

float platform_audio_get_gain(PlatformAudio* a)
{
    return a != NULL ? a->gain : 0.0f;
}

int platform_audio_queued(PlatformAudio* a)
{
    PlatformAudioSubmission* item;
    unsigned long total = 0;
    if (a == NULL) return 0;
    platform_audio_reap(a);
    for (item = a->submissions; item != NULL; item = item->next) {
        DWORD played = 0;
        DWORD queued;
        MMTIME position;
        if (item->is_loop) {
            queued = item->bytes;
        }
        else {
            memset(&position, 0, sizeof(position));
            position.wType = TIME_BYTES;
            if (waveOutGetPosition(item->output, &position,
                                   sizeof(position)) == MMSYSERR_NOERROR) {
                if (position.wType == TIME_BYTES) played = position.u.cb;
                else if (position.wType == TIME_MS) {
                    played = (DWORD)((double)position.u.ms *
                        (double)item->sample_rate * item->channels * 2.0 /
                        1000.0);
                }
            }
            if (played > item->bytes) played = item->bytes;
            queued = item->bytes - played;
        }
        total += queued;
        if (total > 0x7fffffffUL) return 0x7fffffff;
    }
    return (int)total;
}

int platform_audio_queued_milliseconds(PlatformAudio* a)
{
    PlatformAudioSubmission* item;
    double total = 0.0;
    if (a == NULL) return 0;
    platform_audio_reap(a);
    for (item = a->submissions; item != NULL; item = item->next) {
        DWORD played = 0;
        DWORD queued;
        MMTIME position;
        if (item->is_loop) {
            queued = item->bytes;
        }
        else {
            memset(&position, 0, sizeof(position));
            position.wType = TIME_BYTES;
            if (waveOutGetPosition(item->output, &position,
                                   sizeof(position)) == MMSYSERR_NOERROR) {
                if (position.wType == TIME_BYTES) played = position.u.cb;
                else if (position.wType == TIME_MS) {
                    played = (DWORD)((double)position.u.ms *
                        (double)item->sample_rate * item->channels * 2.0 /
                        1000.0);
                }
            }
            if (played > item->bytes) played = item->bytes;
            queued = item->bytes - played;
        }
        total += ((double)queued * 1000.0) /
                 ((double)item->sample_rate * item->channels * 2.0);
    }
    if (total > 0x7fffffff) return 0x7fffffff;
    return (int)total;
}

void platform_audio_stop(PlatformAudio* a)
{
    PlatformAudioSubmission* item;
    if (a == NULL) return;
    item = a->submissions;
    a->submissions = NULL;
    a->loop = NULL;
    while (item != NULL) {
        PlatformAudioSubmission* next = item->next;
        platform_audio_submission_destroy(item, 1);
        item = next;
    }
}

/* Read only volume controls on compact-disc source lines. The retail options
 * getter enumerates every mixer device/destination/source and ignores the
 * PCM output gain; keep those two controls separate in the portable build. */
int platform_cd_mixer_volume_get(float* music, float* sfx)
{
    UINT device_count;
    UINT device_index;
    int found = 0;

    if (music == NULL || sfx == NULL) return 0;
    *music = 0.0f;
    *sfx = 0.0f;
    device_count = mixerGetNumDevs();
    for (device_index = 0; device_index < device_count; ++device_index) {
        HMIXER mixer = NULL;
        MIXERCAPSA caps;
        UINT destination_index;

        if (mixerOpen(&mixer, device_index, 0, 0, CALLBACK_NULL) !=
            MMSYSERR_NOERROR) {
            continue;
        }
        memset(&caps, 0, sizeof(caps));
        if (mixerGetDevCapsA((UINT)device_index, &caps,
                             sizeof(caps)) == MMSYSERR_NOERROR) {
            for (destination_index = 0;
                 destination_index < caps.cDestinations;
                 ++destination_index) {
                MIXERLINEA destination_line;
                DWORD source_index;

                memset(&destination_line, 0, sizeof(destination_line));
                destination_line.cbStruct = sizeof(destination_line);
                destination_line.dwDestination = destination_index;
                if (mixerGetLineInfoA((HMIXEROBJ)mixer, &destination_line,
                                      MIXER_GETLINEINFOF_DESTINATION) !=
                    MMSYSERR_NOERROR) {
                    continue;
                }
                for (source_index = 0;
                     source_index < destination_line.cConnections;
                     ++source_index) {
                    MIXERLINEA source_line;
                    MIXERCONTROL control;
                    MIXERLINECONTROLSA line_controls;
                    MIXERCONTROLDETAILS details;
                    MIXERCONTROLDETAILS_UNSIGNED* values;
                    DWORD channel_count;
                    LONG minimum;
                    LONG maximum;
                    float first;
                    float second;

                    memset(&source_line, 0, sizeof(source_line));
                    source_line.cbStruct = sizeof(source_line);
                    source_line.dwDestination = destination_index;
                    source_line.dwSource = source_index;
                    if (mixerGetLineInfoA((HMIXEROBJ)mixer, &source_line,
                                          MIXER_GETLINEINFOF_SOURCE) !=
                        MMSYSERR_NOERROR ||
                        source_line.dwComponentType !=
                            MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC) {
                        continue;
                    }

                    memset(&control, 0, sizeof(control));
                    memset(&line_controls, 0, sizeof(line_controls));
                    line_controls.cbStruct = sizeof(line_controls);
                    line_controls.dwLineID = source_line.dwLineID;
                    line_controls.dwControlType =
                        MIXERCONTROL_CONTROLTYPE_VOLUME;
                    line_controls.cControls = 1;
                    line_controls.cbmxctrl = sizeof(control);
                    line_controls.pamxctrl = &control;
                    if (mixerGetLineControlsA((HMIXEROBJ)mixer,
                                              &line_controls,
                                              MIXER_GETLINECONTROLSF_ONEBYTYPE)
                        != MMSYSERR_NOERROR ||
                        control.dwControlType !=
                            MIXERCONTROL_CONTROLTYPE_VOLUME) {
                        continue;
                    }

                    channel_count = source_line.cChannels;
                    if (channel_count == 0u ||
                        channel_count > 0x7fffffffu /
                            sizeof(MIXERCONTROLDETAILS_UNSIGNED)) {
                        continue;
                    }
                    values = (MIXERCONTROLDETAILS_UNSIGNED*)calloc(
                        channel_count, sizeof(*values));
                    if (values == NULL) continue;

                    memset(&details, 0, sizeof(details));
                    details.cbStruct = sizeof(details);
                    details.dwControlID = control.dwControlID;
                    details.cChannels = channel_count;
                    details.cMultipleItems = 0;
                    details.cbDetails = sizeof(*values);
                    details.paDetails = values;
                    if (mixerGetControlDetailsA((HMIXEROBJ)mixer, &details,
                                                MIXER_GETCONTROLDETAILSF_VALUE)
                        != MMSYSERR_NOERROR) {
                        free(values);
                        continue;
                    }

                    minimum = control.Bounds.lMinimum;
                    maximum = control.Bounds.lMaximum;
                    if (maximum <= minimum) {
                        free(values);
                        continue;
                    }
                    first = (float)((LONG)values[0].dwValue - minimum) /
                            (float)(maximum - minimum);
                    if (first < 0.0f) first = 0.0f;
                    if (first > 1.0f) first = 1.0f;
                    second = first;
                    if (channel_count == 2u) {
                        second = (float)((LONG)values[1].dwValue - minimum) /
                                 (float)(maximum - minimum);
                        if (second < 0.0f) second = 0.0f;
                        if (second > 1.0f) second = 1.0f;
                    }
                    *music = first;
                    *sfx = second;
                    found = 1;
                    free(values);
                }
            }
        }
        mixerClose((HMIXER)mixer);
    }
    return found;
}

static int s_platform_cd_audio_open;

static int platform_cd_audio_command(const char* command, int* error_code)
{
    MCIERROR result = mciSendStringA(command, NULL, 0, NULL);
    if (error_code != NULL) *error_code = (int)result;
    return result == 0 ? 1 : 0;
}

int platform_cd_audio_open(int* error_code)
{
    int opened;
    if (s_platform_cd_audio_open != 0) {
        if (error_code != NULL) *error_code = 0;
        return 1;
    }
    opened = platform_cd_audio_command("open cdaudio", error_code);
    if (opened) s_platform_cd_audio_open = 1;
    return opened;
}

int platform_cd_audio_set_tmsf(int* error_code)
{
    return platform_cd_audio_command("set cdaudio time format tmsf",
                                     error_code);
}

int platform_cd_audio_play(int one_based_track, int* error_code)
{
    char command[64];
    if (one_based_track < 1) {
        if (error_code != NULL) *error_code = MCIERR_OUTOFRANGE;
        return 0;
    }
    sprintf(command, "play cdaudio from %d", one_based_track);
    return platform_cd_audio_command(command, error_code);
}

int platform_cd_audio_stop(int* error_code)
{
    return platform_cd_audio_command("stop cdaudio", error_code);
}

int platform_cd_audio_close(int* error_code)
{
    int closed = platform_cd_audio_command("close cdaudio", error_code);
    if (closed) s_platform_cd_audio_open = 0;
    return closed;
}

int platform_cd_audio_status(char* mode, int mode_capacity,
                             int* current_track, int* error_code)
{
    char track_text[32];
    MCIERROR result;
    if (mode == NULL || mode_capacity <= 0 || current_track == NULL) {
        if (error_code != NULL) *error_code = MCIERR_PARAM_OVERFLOW;
        return 0;
    }
    mode[0] = '\0';
    result = mciSendStringA("status cdaudio mode", mode,
                            (UINT)mode_capacity, NULL);
    if (result == 0) {
        track_text[0] = '\0';
        result = mciSendStringA("status cdaudio current track", track_text,
                                sizeof(track_text), NULL);
    }
    if (result == 0) *current_track = atoi(track_text);
    if (error_code != NULL) *error_code = (int)result;
    return result == 0 ? 1 : 0;
}

int platform_cd_audio_error_text(int error_code, char* output,
                                 int output_capacity)
{
    if (output == NULL || output_capacity <= 0) return 0;
    return mciGetErrorStringA((MCIERROR)error_code, output,
                              (UINT)output_capacity) ? 1 : 0;
}

uint32_t platform_time_milliseconds(void)
{
    return (uint32_t)timeGetTime();
}
