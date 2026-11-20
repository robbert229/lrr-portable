/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Seam recorder injection: scenarios, the virtual clock, scripted input,
 * window-message capture/delivery, DirectInput and DirectSound readings, and
 * framebuffer screenshots. See ../README.md ("Scenarios"). */
#include "sr.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

unsigned sr_frame;

/* ------------------------------------------------------------------------- */
/* scenario file                                                             */

#define SR_NCHAN 7
static const char* const sr_chan_names[SR_NCHAN] = { "time", "cursor", "di", "ds", "msg", "av", "rnd" };

typedef struct SRChan { char** lines; int n, cap, pos; } SRChan;
static SRChan g_chan[SR_NCHAN];
static HANDLE g_scn_out = INVALID_HANDLE_VALUE;
static SRBuf g_scn_buf;

static int sr_chan_index(const char* chan)
{
    for (int i = 0; i < SR_NCHAN; i++)
        if (!strcmp(sr_chan_names[i], chan)) return i;
    return -1;
}

static char* sr_read_file(const char* env)
{
    char path[1024];
    DWORD n = GetEnvironmentVariableA(env, path, sizeof path);
    if (!n || n >= sizeof path) return NULL;
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        sr_side_warn("FATAL: cannot open %s=%s", env, path);
        sr_flush();
        ExitProcess(98);
    }
    DWORD size = GetFileSize(h, NULL), got = 0;
    char* p = (char*)malloc(size + 1);
    ReadFile(h, p, size, &got, NULL);
    CloseHandle(h);
    p[got] = 0;
    return p;
}

void sr_divergence(const char* what)
{
    sr_log_warn("SCENARIO DIVERGENCE at frame %u: %s", sr_frame, what);
    sr_shutdown("divergence");
    TerminateProcess(GetCurrentProcess(), 3);
    for (;;) Sleep(1000);
}

int sr_scn_next(const char* chan, const char* key, char* out, int outsz)
{
    SRChan* c = &g_chan[sr_chan_index(chan)];
    if (c->pos >= c->n) {
        char tmp[160];
        snprintf(tmp, sizeof tmp, "scenario channel '%s' exhausted (wanted %s)", chan, key);
        sr_divergence(tmp);
    }
    const char* line = c->lines[c->pos++];
    size_t kl = strlen(key);
    if (strncmp(line, key, kl) || (line[kl] != ' ' && line[kl] != 0)) {
        char tmp[300];
        snprintf(tmp, sizeof tmp, "scenario '%s' reading #%d is '%.120s', the game asked for '%s'", chan, c->pos,
                 line, key);
        sr_divergence(tmp);
    }
    snprintf(out, outsz, "%s", line[kl] ? line + kl + 1 : "");
    return 1;
}

void sr_scn_write(const char* chan, const char* key, const char* fmt, ...)
{
    if (g_scn_out == INVALID_HANDLE_VALUE) return;
    sr_putf(&g_scn_buf, "%s %s ", chan, key);
    char tmp[1200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    sr_put(&g_scn_buf, tmp);
    sr_put(&g_scn_buf, "\n");
    if (g_scn_buf.len > 65536) {
        DWORD w;
        WriteFile(g_scn_out, g_scn_buf.p, g_scn_buf.len, &w, NULL);
        g_scn_buf.len = 0;
    }
}

static void sr_scn_flush(void)
{
    DWORD w;
    if (g_scn_out != INVALID_HANDLE_VALUE && g_scn_buf.len) WriteFile(g_scn_out, g_scn_buf.p, g_scn_buf.len, &w, NULL);
    g_scn_buf.len = 0;
}

/* -- script ------------------------------------------------------------------ */

typedef struct SREvent { unsigned frame; char what[16]; int a, b; } SREvent;
static SREvent* g_ev;
static int g_nev, g_evpos;
static unsigned g_frame_ms = 40, g_vtime = 100000, g_vbase = 100000, g_reads_this_frame, g_stop_after;
static int g_cursor_x = 320, g_cursor_y = 240;
static unsigned char g_keys[256];

static void sr_script_load(char* text)
{
    for (char* line = strtok(text, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        char* hash = strchr(line, '#');
        if (hash) *hash = 0;
        char w0[32] = "", w1[32] = "", w2[32] = "", w3[32] = "";
        unsigned f;
        int n = sscanf(line, "%31s %31s %31s %31s", w0, w1, w2, w3);
        if (n <= 0) continue;
        if (!strcmp(w0, "frame_ms")) g_frame_ms = atoi(w1);
        else if (!strcmp(w0, "start_time")) g_vbase = g_vtime = strtoul(w1, NULL, 0);
        else if (!strcmp(w0, "stop_after")) g_stop_after = atoi(w1);
        else if (!strcmp(w0, "args")) continue; /* game command line: seamrec.py passes it */
        else if (!strcmp(w0, "cursor")) { g_cursor_x = atoi(w1); g_cursor_y = atoi(w2); }
        else if (!strcmp(w0, "at") && sscanf(w1, "%u", &f) == 1) {
            g_ev = (SREvent*)realloc(g_ev, (g_nev + 1) * sizeof *g_ev);
            SREvent* e = &g_ev[g_nev++];
            memset(e, 0, sizeof *e);
            e->frame = f;
            snprintf(e->what, sizeof e->what, "%s", w2);
            char rest[64] = "";
            /* operands: cursor X Y | key DIK down|up | lbutton/rbutton down|up | close */
            const char* p = strstr(line, w2) + strlen(w2);
            snprintf(rest, sizeof rest, "%s", p);
            char o1[16] = "", o2[16] = "";
            sscanf(rest, "%15s %15s", o1, o2);
            if (!strcmp(e->what, "cursor")) { e->a = atoi(o1); e->b = atoi(o2); }
            else if (!strcmp(e->what, "key")) { e->a = (int)strtol(o1, NULL, 0); e->b = !strcmp(o2, "down"); }
            else if (!strcmp(e->what, "lbutton") || !strcmp(e->what, "rbutton")) e->b = !strcmp(o1, "down");
            else if (strcmp(e->what, "close")) {
                sr_side_warn("FATAL: unknown script event '%s'", e->what);
                ExitProcess(98);
            }
        } else {
            sr_side_warn("FATAL: unknown script line '%s'", line);
            ExitProcess(98);
        }
    }
}

void sr_scn_init(void)
{
    sr_buf_init(&g_scn_buf);
    char tmp[16];
    if (GetEnvironmentVariableA("SEAMREC_MAXFRAMES", tmp, sizeof tmp)) g_stop_after = atoi(tmp);
    if (sr_mode == SR_REPLAY) {
        char* text = sr_read_file("SEAMREC_SCENARIO");
        if (!text) {
            sr_side_warn("FATAL: replay mode needs SEAMREC_SCENARIO");
            ExitProcess(98);
        }
        for (char* line = strtok(text, "\r\n"); line; line = strtok(NULL, "\r\n")) {
            char chan[16];
            if (line[0] == '#' || sscanf(line, "%15s", chan) != 1) continue;
            /* the script's stop frame, unless SEAMREC_MAXFRAMES overrides it */
            if (!strcmp(chan, "stop_after")) {
                if (!g_stop_after) g_stop_after = atoi(line + strlen(chan));
                continue;
            }
            int ci = sr_chan_index(chan);
            if (ci < 0) continue; /* header, end marker */
            SRChan* c = &g_chan[ci];
            if (c->n == c->cap) {
                c->cap = c->cap ? c->cap * 2 : 1024;
                c->lines = (char**)realloc(c->lines, c->cap * sizeof(char*));
            }
            c->lines[c->n++] = line + strlen(chan) + 1;
        }
    } else if (sr_mode == SR_RECORD || sr_mode == SR_SCRIPT) {
        char path[1024];
        if (GetEnvironmentVariableA("SEAMREC_SCENARIO", path, sizeof path))
            g_scn_out = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
        sr_put(&g_scn_buf, "seamrec-scenario 1\n");
        /* the game's arguments, so a replay launches it identically */
        const char* cl = GetCommandLineA();
        const char* exe = strstr(cl, "LegoRR.exe");
        const char* rest = exe ? exe + strlen("LegoRR.exe") : "";
        while (*rest == '"' || *rest == ' ') rest++;
        sr_putf(&g_scn_buf, "args %s\n", rest);
        if (sr_mode == SR_SCRIPT) {
            char* text = sr_read_file("SEAMREC_SCRIPT");
            if (!text) {
                sr_side_warn("FATAL: script mode needs SEAMREC_SCRIPT");
                ExitProcess(98);
            }
            sr_script_load(text);
            if (g_stop_after) sr_putf(&g_scn_buf, "stop_after %u\n", g_stop_after);
            sr_put(&g_scn_buf, "# scripted\n");
        }
    }
}

void sr_scn_finish(void)
{
    if (sr_mode == SR_REPLAY) {
        for (int i = 0; i < SR_NCHAN; i++)
            if (g_chan[i].pos < g_chan[i].n)
                sr_side_warn("replay: %d unconsumed '%s' readings", g_chan[i].n - g_chan[i].pos, sr_chan_names[i]);
    }
    if (g_scn_out != INVALID_HANDLE_VALUE) {
        sr_putf(&g_scn_buf, "end %u\n", sr_frame);
        sr_scn_flush();
        CloseHandle(g_scn_out);
        g_scn_out = INVALID_HANDLE_VALUE;
    }
}

/* ------------------------------------------------------------------------- */
/* window messages                                                           */

static WNDPROC g_game_proc;
static HWND g_hwnd;
typedef struct SRMsg { unsigned tag; UINT msg; WPARAM w; LPARAM l; } SRMsg;
static SRMsg* g_pending;
static int g_npending, g_cappending, g_delivering;

/* Messages that change game state (window_game_window_procedure_retail and
 * window_activation_close_retail in src/gods98/main.c). Everything else the
 * game's procedure forwards to DefWindowProcA or ignores. */
static int sr_msg_relevant(UINT m)
{
    switch (m) {
    case WM_ACTIVATEAPP: case WM_CLOSE: case WM_DESTROY: case WM_QUIT:
    case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
        return 1;
    }
    return 0;
}

static void sr_queue_msg(unsigned tag, UINT m, WPARAM w, LPARAM l)
{
    if (g_npending == g_cappending) {
        g_cappending = g_cappending ? g_cappending * 2 : 64;
        g_pending = (SRMsg*)realloc(g_pending, g_cappending * sizeof(SRMsg));
    }
    SRMsg* p = &g_pending[g_npending++];
    p->tag = tag;
    p->msg = m;
    p->w = w;
    p->l = l;
    char key[16];
    snprintf(key, sizeof key, "%u", tag);
    sr_scn_write("msg", key, "0x%04x 0x%lx 0x%lx", m, (unsigned long)w, (unsigned long)l);
}

static const char* sr_msg_name(UINT m)
{
    switch (m) {
    case WM_ACTIVATEAPP: return "WM_ACTIVATEAPP";
    case WM_CLOSE: return "WM_CLOSE";
    case WM_DESTROY: return "WM_DESTROY";
    case WM_QUIT: return "WM_QUIT";
    case WM_LBUTTONDOWN: return "WM_LBUTTONDOWN";
    case WM_LBUTTONUP: return "WM_LBUTTONUP";
    case WM_LBUTTONDBLCLK: return "WM_LBUTTONDBLCLK";
    case WM_RBUTTONDOWN: return "WM_RBUTTONDOWN";
    case WM_RBUTTONUP: return "WM_RBUTTONUP";
    case WM_RBUTTONDBLCLK: return "WM_RBUTTONDBLCLK";
    }
    return NULL;
}

static void sr_call_game_proc(UINT m, WPARAM w, LPARAM l)
{
    SRCall c;
    sr_begin_cb(&c, "WndProc");
    LRESULT r = CallWindowProcA(g_game_proc, g_hwnd, m, w, l);
    if (c.log) {
        sr_f_handle(&c.b, g_hwnd);
        const char* name = sr_msg_name(m);
        if (name) sr_putf(&c.b, ", %s", name);
        else sr_putf(&c.b, ", 0x%04x", m);
        /* WM_ACTIVATEAPP's lParam is a thread id: not part of the seam */
        if (m == WM_ACTIVATEAPP) sr_putf(&c.b, ", %u) = %ld", (unsigned)w, (long)r);
        else sr_putf(&c.b, ", 0x%x, 0x%lx) = %ld", (unsigned)w, (unsigned long)l, (long)r);
    }
    sr_end(&c);
}

/* Called at the end of every top-level logged call: hand the game the
 * relevant messages that arrived during that call (record/script), or the
 * recorded ones (replay), so they always land at the same point. */
void sr_deliver_messages(unsigned seq)
{
    if (g_delivering || !g_game_proc || sr_mode == SR_PASS) return;
    g_delivering = 1;
    if (sr_mode == SR_REPLAY) {
        SRChan* c = &g_chan[sr_chan_index("msg")];
        while (c->pos < c->n) {
            unsigned tag, m;
            unsigned long w, l;
            if (sscanf(c->lines[c->pos], "%u 0x%x 0x%lx 0x%lx", &tag, &m, &w, &l) != 4) {
                sr_divergence("malformed msg reading");
            }
            if (tag > seq) break;
            if (tag < seq) sr_log_warn("replay: message for call %u delivered late at call %u", tag, seq);
            c->pos++;
            sr_call_game_proc(m, (WPARAM)w, (LPARAM)l);
        }
    } else {
        while (g_npending && g_pending[0].tag <= seq) {
            SRMsg m = g_pending[0];
            memmove(g_pending, g_pending + 1, (--g_npending) * sizeof(SRMsg));
            sr_call_game_proc(m.msg, m.w, m.l);
        }
    }
    g_delivering = 0;
}

static LRESULT CALLBACK sr_wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (!g_hwnd) g_hwnd = h;
    if (sr_msg_relevant(m) && h == g_hwnd && sr_mode != SR_PASS) {
        if (sr_mode != SR_REPLAY) sr_queue_msg(sr_top_seq(), m, w, l);
        return 0; /* the game gets it (or its recording) at the end of the current call */
    }
    if (sr_mode == SR_PASS && sr_msg_relevant(m)) {
        sr_call_game_proc(m, w, l);
        return 0;
    }
    sr_suppress++;
    LRESULT r = CallWindowProcA(g_game_proc, h, m, w, l);
    sr_suppress--;
    return r;
}

void sr_pump_messages(void)
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) continue;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

ATOM WINAPI SRH_RegisterClassA(const WNDCLASSA* wc)
{
    WNDCLASSA copy = *wc;
    g_game_proc = wc->lpfnWndProc;
    copy.lpfnWndProc = sr_wndproc;
    return sr_real_RegisterClassA(&copy);
}

/* ------------------------------------------------------------------------- */
/* frames, time                                                              */

void sr_on_frame(void)
{
    sr_frame++;
    if (sr_mode == SR_SCRIPT) {
        unsigned next = g_vbase + g_frame_ms;
        if (next <= g_vtime) next = g_vtime + 1;
        g_vbase = g_vtime = next;
        g_reads_this_frame = 0;
        while (g_evpos < g_nev && g_ev[g_evpos].frame <= sr_frame) {
            SREvent* e = &g_ev[g_evpos++];
            LPARAM pos = MAKELPARAM(g_cursor_x, g_cursor_y);
            if (!strcmp(e->what, "cursor")) { g_cursor_x = e->a; g_cursor_y = e->b; }
            else if (!strcmp(e->what, "key")) g_keys[e->a & 255] = e->b ? 0x80 : 0;
            else if (!strcmp(e->what, "lbutton")) sr_queue_msg(sr_top_seq(), e->b ? WM_LBUTTONDOWN : WM_LBUTTONUP, e->b ? MK_LBUTTON : 0, pos);
            else if (!strcmp(e->what, "rbutton")) sr_queue_msg(sr_top_seq(), e->b ? WM_RBUTTONDOWN : WM_RBUTTONUP, e->b ? MK_RBUTTON : 0, pos);
            else if (!strcmp(e->what, "close")) sr_queue_msg(sr_top_seq(), WM_CLOSE, 0, 0);
        }
    }
    sr_flush();
    if (g_stop_after && sr_frame > g_stop_after) {
        sr_log_warn("stop_after/SEAMREC_MAXFRAMES reached at frame %u", sr_frame);
        sr_shutdown("maxframes");
        TerminateProcess(GetCurrentProcess(), 4);
    }
}

DWORD WINAPI SRH_timeGetTime(void)
{
    char buf[64];
    DWORD v;
    switch (sr_mode) {
    case SR_REPLAY:
        sr_scn_next("time", "t", buf, sizeof buf);
        return strtoul(buf, NULL, 10);
    case SR_SCRIPT:
        /* a spinning wait loop that never pumps still sees time pass */
        v = g_vtime + (g_reads_this_frame++ / 16);
        if (v > g_vtime) g_vtime = v;
        sr_scn_write("time", "t", "%lu", (unsigned long)v);
        return v;
    case SR_RECORD:
        v = sr_real_timeGetTime();
        sr_scn_write("time", "t", "%lu", (unsigned long)v);
        return v;
    default:
        return sr_real_timeGetTime();
    }
}

/* Microsoft's D3DRM seeds its own generator for D3DRMVectorRandom, so its
 * vectors differ from run to run. Script mode synthesizes them from a fixed
 * seed. Readings hold each component's float32 bits, so a replay returns
 * exactly the recorded vector. */
static void sr_rnd_write(const D3DVECTOR* v)
{
    uint32_t b[3];
    memcpy(&b[0], &v->x, 4);
    memcpy(&b[1], &v->y, 4);
    memcpy(&b[2], &v->z, 4);
    sr_scn_write("rnd", "v", "%08x %08x %08x", (unsigned)b[0], (unsigned)b[1], (unsigned)b[2]);
}

D3DVECTOR* WINAPI SRH_D3DRMVectorRandom(D3DVECTOR* ret)
{
    static uint32_t g_rnd_state = 12345u;
    char buf[128];
    unsigned b[3];
    switch (sr_mode) {
    case SR_REPLAY:
        sr_scn_next("rnd", "v", buf, sizeof buf);
        if (sscanf(buf, "%x %x %x", &b[0], &b[1], &b[2]) != 3) sr_divergence("malformed rnd reading");
        memcpy(&ret->x, &b[0], 4);
        memcpy(&ret->y, &b[1], 4);
        memcpy(&ret->z, &b[2], 4);
        return ret;
    case SR_SCRIPT: {
        float c[3], l2;
        do {
            for (int i = 0; i < 3; i++) {
                g_rnd_state = g_rnd_state * 1103515245u + 12345u;
                c[i] = (float)((g_rnd_state >> 8) & 0xffff) / 32768.0f - 1.0f;
            }
            l2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
        } while (l2 < 1e-4f || l2 > 1.0f);
        float inv = 1.0f / sqrtf(l2);
        ret->x = c[0] * inv;
        ret->y = c[1] * inv;
        ret->z = c[2] * inv;
        sr_rnd_write(ret);
        return ret;
    }
    case SR_RECORD:
        sr_real_D3DRMVectorRandom(ret);
        sr_rnd_write(ret);
        return ret;
    default:
        return sr_real_D3DRMVectorRandom(ret);
    }
}

WINBOOL WINAPI SRH_PeekMessageA(LPMSG msg, HWND h, UINT lo, UINT hi, UINT rm)
{
    if (sr_mode == SR_PASS) {
        sr_on_frame();
        return sr_real_PeekMessageA(msg, h, lo, hi, rm);
    }
    /* The game always sees an empty queue: real messages are dispatched
     * here, and the state-changing ones reach it through
     * sr_deliver_messages at the end of this call. */
    sr_on_frame();
    sr_pump_messages();
    if (msg) memset(msg, 0, sizeof *msg);
    return FALSE;
}

void WINAPI SRH_Sleep(DWORD ms)
{
    sr_real_Sleep(sr_mode == SR_SCRIPT || sr_mode == SR_REPLAY ? (ms ? 1 : 0) : ms);
}

void WINAPI SRH_ExitProcess(UINT code)
{
    sr_shutdown("ExitProcess");
    sr_real_ExitProcess(code);
}

/* ------------------------------------------------------------------------- */
/* cursor                                                                    */

WINBOOL WINAPI SRH_GetCursorPos(LPPOINT p)
{
    char buf[64];
    WINBOOL r;
    int x, y;
    switch (sr_mode) {
    case SR_REPLAY:
        sr_scn_next("cursor", "pos", buf, sizeof buf);
        if (sscanf(buf, "%d %d %d", &r, &x, &y) != 3) sr_divergence("malformed cursor reading");
        if (p) { p->x = x; p->y = y; }
        return r;
    case SR_SCRIPT:
        if (p) { p->x = g_cursor_x; p->y = g_cursor_y; }
        sr_scn_write("cursor", "pos", "1 %d %d", g_cursor_x, g_cursor_y);
        return TRUE;
    case SR_RECORD:
        r = sr_real_GetCursorPos(p);
        sr_scn_write("cursor", "pos", "%d %ld %ld", r, p ? p->x : 0L, p ? p->y : 0L);
        return r;
    default:
        return sr_real_GetCursorPos(p);
    }
}

WINBOOL WINAPI SRH_SetCursorPos(int x, int y)
{
    if (sr_mode == SR_SCRIPT) { g_cursor_x = x; g_cursor_y = y; }
    return sr_real_SetCursorPos(x, y);
}

/* ------------------------------------------------------------------------- */
/* DirectInput                                                               */

static HRESULT sr_inject_hr(SRWrap* w, const char* method, HRESULT hr)
{
    char key[64], buf[64];
    snprintf(key, sizeof key, "%s #%u", method, w->id);
    switch (sr_mode) {
    case SR_REPLAY:
        sr_scn_next("di", key, buf, sizeof buf);
        return (HRESULT)strtoul(buf, NULL, 0);
    case SR_SCRIPT:
        hr = DI_OK;
        /* fallthrough */
    case SR_RECORD:
        sr_scn_write("di", key, "0x%08lx", (unsigned long)hr);
        return hr;
    }
    return hr;
}

static HRESULT sr_inject_state(SRWrap* w, HRESULT hr, DWORD cb, void* data)
{
    char key[64];
    unsigned char* d = (unsigned char*)data;
    snprintf(key, sizeof key, "GetDeviceState #%u", w->id);
    if (sr_mode == SR_PASS || !d) return hr;
    if (sr_mode == SR_REPLAY) {
        char buf[2048];
        sr_scn_next("di", key, buf, sizeof buf);
        char* p = buf;
        hr = (HRESULT)strtoul(p, &p, 0);
        memset(d, 0, cb);
        unsigned idx, val;
        int n;
        while (sscanf(p, " %x:%x%n", &idx, &val, &n) == 2) {
            if (idx < cb) d[idx] = (unsigned char)val;
            p += n;
        }
        return hr;
    }
    if (sr_mode == SR_SCRIPT) {
        hr = DI_OK;
        memset(d, 0, cb);
        if (cb == 256) memcpy(d, g_keys, 256);
    }
    SRBuf b;
    sr_buf_init(&b);
    sr_putf(&b, "0x%08lx", (unsigned long)hr);
    for (DWORD i = 0; i < cb; i++)
        if (d[i]) sr_putf(&b, " %lx:%x", (unsigned long)i, d[i]);
    sr_scn_write("di", key, "%s", b.p);
    sr_buf_free(&b);
    return hr;
}

#define SR_DI_HOOKS(I)                                                                              \
    HRESULT SRH_##I##_GetDeviceState(SRWrap* w, DWORD cb, LPVOID data)                             \
    {                                                                                               \
        I* r = (I*)w->real;                                                                         \
        return sr_inject_state(w, r->lpVtbl->GetDeviceState(r, cb, data), cb, data);                \
    }                                                                                               \
    HRESULT SRH_##I##_Acquire(SRWrap* w)                                                            \
    {                                                                                               \
        I* r = (I*)w->real;                                                                         \
        return sr_inject_hr(w, "Acquire", r->lpVtbl->Acquire(r));                                   \
    }                                                                                               \
    HRESULT SRH_##I##_Unacquire(SRWrap* w)                                                          \
    {                                                                                               \
        I* r = (I*)w->real;                                                                         \
        return sr_inject_hr(w, "Unacquire", r->lpVtbl->Unacquire(r));                               \
    }
SR_DI_HOOKS(IDirectInputDeviceA)
SR_DI_HOOKS(IDirectInputDevice2A)

/* ------------------------------------------------------------------------- */
/* DirectSound: positions and status are readings (they move in real time).  */

typedef struct SRDSBuf {
    void* real;
    DWORD bytes, avg, block, freq0, freq, start_pos;
    unsigned start_ms;
    int playing, looping;
    struct SRDSBuf* next;
} SRDSBuf;
static SRDSBuf* g_dsbufs;

static SRDSBuf* sr_dsbuf(void* real, int create)
{
    for (SRDSBuf* b = g_dsbufs; b; b = b->next)
        if (b->real == real) return b;
    if (!create) return NULL;
    SRDSBuf* b = (SRDSBuf*)calloc(1, sizeof *b);
    b->real = real;
    b->next = g_dsbufs;
    g_dsbufs = b;
    return b;
}

/* script model: position advances with the virtual clock */
static DWORD sr_dsbuf_pos(SRDSBuf* b)
{
    if (!b->playing || !b->avg) return b->start_pos;
    double rate = (double)b->avg * (b->freq && b->freq0 ? (double)b->freq / b->freq0 : 1.0);
    unsigned long long adv = (unsigned long long)((double)(g_vtime - b->start_ms) * rate / 1000.0);
    unsigned long long pos = b->start_pos + adv;
    if (b->block > 1) pos -= pos % b->block;
    if (b->looping) return b->bytes ? (DWORD)(pos % b->bytes) : 0;
    if (pos >= b->bytes) { b->playing = 0; b->start_pos = 0; return 0; }
    return (DWORD)pos;
}

static void sr_dsbuf_rebase(SRDSBuf* b)
{
    b->start_pos = sr_dsbuf_pos(b);
    b->start_ms = g_vtime;
}

HRESULT SRH_IDirectSound_CreateSoundBuffer(SRWrap* w, LPCDSBUFFERDESC desc, LPLPDIRECTSOUNDBUFFER out, LPUNKNOWN outer)
{
    IDirectSound* r = (IDirectSound*)w->real;
    HRESULT hr = r->lpVtbl->CreateSoundBuffer(r, desc, out, outer);
    if (SUCCEEDED(hr) && out && *out && desc) {
        SRDSBuf* b = sr_dsbuf(*out, 1);
        b->bytes = desc->dwBufferBytes;
        if (desc->lpwfxFormat) {
            b->avg = desc->lpwfxFormat->nAvgBytesPerSec;
            b->block = desc->lpwfxFormat->nBlockAlign;
            b->freq0 = desc->lpwfxFormat->nSamplesPerSec;
        }
    }
    return hr;
}

HRESULT SRH_IDirectSound_DuplicateSoundBuffer(SRWrap* w, LPDIRECTSOUNDBUFFER src, LPLPDIRECTSOUNDBUFFER out)
{
    IDirectSound* r = (IDirectSound*)w->real;
    HRESULT hr = r->lpVtbl->DuplicateSoundBuffer(r, src, out);
    if (SUCCEEDED(hr) && out && *out) {
        SRDSBuf* s = sr_dsbuf(src, 0);
        SRDSBuf* b = sr_dsbuf(*out, 1);
        if (s) { b->bytes = s->bytes; b->avg = s->avg; b->block = s->block; b->freq0 = s->freq0; }
    }
    return hr;
}

HRESULT SRH_IDirectSoundBuffer_Play(SRWrap* w, DWORD reserved1, DWORD reserved2, DWORD flags)
{
    IDirectSoundBuffer* r = (IDirectSoundBuffer*)w->real;
    HRESULT hr = r->lpVtbl->Play(r, reserved1, reserved2, flags);
    SRDSBuf* b = sr_dsbuf(w->real, 1);
    sr_dsbuf_rebase(b);
    b->playing = 1;
    b->looping = (flags & DSBPLAY_LOOPING) != 0;
    return hr;
}

HRESULT SRH_IDirectSoundBuffer_Stop(SRWrap* w)
{
    IDirectSoundBuffer* r = (IDirectSoundBuffer*)w->real;
    SRDSBuf* b = sr_dsbuf(w->real, 1);
    sr_dsbuf_rebase(b);
    b->playing = 0;
    return r->lpVtbl->Stop(r);
}

HRESULT SRH_IDirectSoundBuffer_SetCurrentPosition(SRWrap* w, DWORD pos)
{
    IDirectSoundBuffer* r = (IDirectSoundBuffer*)w->real;
    SRDSBuf* b = sr_dsbuf(w->real, 1);
    b->start_pos = pos;
    b->start_ms = g_vtime;
    return r->lpVtbl->SetCurrentPosition(r, pos);
}

HRESULT SRH_IDirectSoundBuffer_SetFrequency(SRWrap* w, DWORD freq)
{
    IDirectSoundBuffer* r = (IDirectSoundBuffer*)w->real;
    SRDSBuf* b = sr_dsbuf(w->real, 1);
    sr_dsbuf_rebase(b);
    b->freq = freq;
    return r->lpVtbl->SetFrequency(r, freq);
}

HRESULT SRH_IDirectSoundBuffer_GetCurrentPosition(SRWrap* w, LPDWORD play, LPDWORD write)
{
    IDirectSoundBuffer* r = (IDirectSoundBuffer*)w->real;
    char key[64], buf[96];
    snprintf(key, sizeof key, "GetCurrentPosition #%u", w->id);
    HRESULT hr = DS_OK;
    unsigned long p = 0, wr = 0;
    switch (sr_mode) {
    case SR_REPLAY:
        sr_scn_next("ds", key, buf, sizeof buf);
        if (sscanf(buf, "0x%lx %lu %lu", (unsigned long*)&hr, &p, &wr) != 3) sr_divergence("malformed ds reading");
        if (play) *play = p;
        if (write) *write = wr;
        return hr;
    case SR_SCRIPT: {
        SRDSBuf* b = sr_dsbuf(w->real, 1);
        p = sr_dsbuf_pos(b);
        wr = b->playing && b->bytes ? (p + (b->avg / 100 ? b->avg / 100 : 1)) % b->bytes : p;
        if (b->block > 1) wr -= wr % b->block;
        if (play) *play = p;
        if (write) *write = wr;
        break;
    }
    default:
        hr = r->lpVtbl->GetCurrentPosition(r, play, write);
        if (sr_mode == SR_PASS) return hr;
        p = play ? *play : 0;
        wr = write ? *write : 0;
    }
    sr_scn_write("ds", key, "0x%08lx %lu %lu", (unsigned long)hr, p, wr);
    return hr;
}

HRESULT SRH_IDirectSoundBuffer_GetStatus(SRWrap* w, LPDWORD status)
{
    IDirectSoundBuffer* r = (IDirectSoundBuffer*)w->real;
    char key[64], buf[96];
    snprintf(key, sizeof key, "GetStatus #%u", w->id);
    HRESULT hr = DS_OK;
    unsigned long s = 0;
    switch (sr_mode) {
    case SR_REPLAY:
        sr_scn_next("ds", key, buf, sizeof buf);
        if (sscanf(buf, "0x%lx %lu", (unsigned long*)&hr, &s) != 2) sr_divergence("malformed ds reading");
        if (status) *status = s;
        return hr;
    case SR_SCRIPT: {
        SRDSBuf* b = sr_dsbuf(w->real, 1);
        sr_dsbuf_pos(b);
        s = b->playing ? (DSBSTATUS_PLAYING | (b->looping ? DSBSTATUS_LOOPING : 0)) : 0;
        if (status) *status = s;
        break;
    }
    default:
        hr = r->lpVtbl->GetStatus(r, status);
        if (sr_mode == SR_PASS) return hr;
        s = status ? *status : 0;
    }
    sr_scn_write("ds", key, "0x%08lx %lu", (unsigned long)hr, s);
    return hr;
}

/* ------------------------------------------------------------------------- */
/* Movie samples: AMStream decodes on its own clock, so whether Update hands  */
/* the game a frame or end-of-stream is a reading, like a sound position.     */

static HRESULT sr_reading_av(SRWrap* w, const char* method, HRESULT hr)
{
    char key[64], buf[64];
    snprintf(key, sizeof key, "%s #%u", method, w->id);
    switch (sr_mode) {
    case SR_REPLAY:
        sr_scn_next("av", key, buf, sizeof buf);
        return (HRESULT)strtoul(buf, NULL, 0);
    case SR_SCRIPT:
    case SR_RECORD:
        sr_scn_write("av", key, "0x%08lx", (unsigned long)hr);
    }
    return hr;
}

#define SR_SAMPLE_HOOKS(I)                                                                          \
    HRESULT SRH_##I##_Update(SRWrap* w, DWORD flags, HANDLE event, PAPCFUNC func, DWORD_PTR context) \
    {                                                                                               \
        I* r = (I*)w->real;                                                                         \
        return sr_reading_av(w, "Update", r->lpVtbl->Update(r, flags, event, func, context));       \
    }                                                                                               \
    HRESULT SRH_##I##_CompletionStatus(SRWrap* w, DWORD flags, DWORD milliseconds)                  \
    {                                                                                               \
        I* r = (I*)w->real;                                                                         \
        return sr_reading_av(w, "CompletionStatus", r->lpVtbl->CompletionStatus(r, flags, milliseconds)); \
    }
SR_SAMPLE_HOOKS(IDirectDrawStreamSample)
SR_SAMPLE_HOOKS(IStreamSample)

/* ------------------------------------------------------------------------- */
/* screenshots: SEAMREC_SHOTS="f1,f2,..." and/or SEAMREC_SHOTEVERY=N into     */
/* SEAMREC_SHOTDIR, taken from the back buffer just before the game's Flip.   */

static int sr_shot_wanted(void)
{
    static int init, every;
    static char list[1024];
    if (!init) {
        init = 1;
        char tmp[16] = "";
        if (GetEnvironmentVariableA("SEAMREC_SHOTEVERY", tmp, sizeof tmp)) every = atoi(tmp);
        if (!GetEnvironmentVariableA("SEAMREC_SHOTS", list + 1, sizeof list - 2)) list[1] = 0;
        list[0] = ',';
        strcat(list, ",");
    }
    if (every && sr_frame % every == 0) return 1;
    char key[24];
    snprintf(key, sizeof key, ",%u,", sr_frame);
    return strstr(list, key) != NULL;
}

static void sr_write_bmp(const DDSURFACEDESC2* d)
{
    char dir[900], path[1024];
    if (!GetEnvironmentVariableA("SEAMREC_SHOTDIR", dir, sizeof dir)) return;
    snprintf(path, sizeof path, "%s\\frame_%06u.bmp", dir, sr_frame);
    int w = d->dwWidth, h = d->dwHeight, bpp = d->ddpfPixelFormat.dwRGBBitCount;
    if (bpp != 16 && bpp != 32 && bpp != 24) return;
    int stride = (w * 3 + 3) & ~3;
    BITMAPFILEHEADER fh = { 0 };
    BITMAPINFOHEADER ih = { 0 };
    fh.bfType = 0x4d42;
    fh.bfOffBits = sizeof fh + sizeof ih;
    fh.bfSize = fh.bfOffBits + stride * h;
    ih.biSize = sizeof ih;
    ih.biWidth = w;
    ih.biHeight = h;
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    unsigned char* rows = (unsigned char*)calloc(stride, h);
    DWORD rm = d->ddpfPixelFormat.dwRBitMask, gm = d->ddpfPixelFormat.dwGBitMask, bm = d->ddpfPixelFormat.dwBBitMask;
    for (int y = 0; y < h; y++) {
        const unsigned char* src = (const unsigned char*)d->lpSurface + y * d->lPitch;
        unsigned char* dst = rows + (h - 1 - y) * stride;
        for (int x = 0; x < w; x++) {
            DWORD px = bpp == 16 ? ((const WORD*)src)[x] : bpp == 32 ? ((const DWORD*)src)[x]
                                                                     : (DWORD)(src[3 * x] | src[3 * x + 1] << 8 | src[3 * x + 2] << 16);
            DWORD m[3] = { bm, gm, rm };
            for (int c = 0; c < 3; c++) {
                DWORD mask = m[c];
                if (!mask) continue;
                int shift = 0;
                while (!((mask >> shift) & 1)) shift++;
                DWORD v = (px & mask) >> shift, max = mask >> shift;
                dst[3 * x + c] = (unsigned char)(v * 255 / max);
            }
        }
    }
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    DWORD wr;
    WriteFile(f, &fh, sizeof fh, &wr, NULL);
    WriteFile(f, &ih, sizeof ih, &wr, NULL);
    WriteFile(f, rows, stride * h, &wr, NULL);
    CloseHandle(f);
    free(rows);
}

static void sr_shot(void* real_surface_any)
{
    if (!sr_shot_wanted()) return;
    IUnknown* u = (IUnknown*)real_surface_any;
    IDirectDrawSurface4* s4 = NULL;
    if (FAILED(u->lpVtbl->QueryInterface(u, &IID_IDirectDrawSurface4, (void**)&s4))) return;
    DDSCAPS2 caps = { 0 };
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    IDirectDrawSurface4* back = NULL;
    if (SUCCEEDED(s4->lpVtbl->GetAttachedSurface(s4, &caps, &back))) {
        DDSURFACEDESC2 d;
        memset(&d, 0, sizeof d);
        d.dwSize = sizeof d;
        if (SUCCEEDED(back->lpVtbl->Lock(back, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL))) {
            sr_write_bmp(&d);
            back->lpVtbl->Unlock(back, NULL);
        }
        back->lpVtbl->Release(back);
    }
    s4->lpVtbl->Release(s4);
}

#define SR_FLIP_HOOK(I, T)                                                                          \
    HRESULT SRH_##I##_Flip(SRWrap* w, T target, DWORD flags)                                        \
    {                                                                                               \
        I* r = (I*)w->real;                                                                         \
        sr_shot(r);                                                                                 \
        return r->lpVtbl->Flip(r, target, flags);                                                   \
    }
SR_FLIP_HOOK(IDirectDrawSurface, LPDIRECTDRAWSURFACE)
SR_FLIP_HOOK(IDirectDrawSurface2, LPDIRECTDRAWSURFACE2)
SR_FLIP_HOOK(IDirectDrawSurface3, LPDIRECTDRAWSURFACE3)
SR_FLIP_HOOK(IDirectDrawSurface4, LPDIRECTDRAWSURFACE4)
