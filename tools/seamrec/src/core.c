/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Seam recorder core: log file, call gate, object ids, wrapper identity,
 * value formatting, closures and real-DLL loading. */
#include "sr.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>

int sr_mode = SR_PASS;
int sr_suppress = 0;

/* ------------------------------------------------------------------------- */
/* buffers                                                                   */

void sr_buf_init(SRBuf* b) { b->p = NULL; b->len = b->cap = 0; }
void sr_buf_free(SRBuf* b) { free(b->p); sr_buf_init(b); }

static void sr_grow(SRBuf* b, int extra)
{
    if (b->len + extra + 1 <= b->cap) return;
    int cap = b->cap ? b->cap : 128;
    while (cap < b->len + extra + 1) cap *= 2;
    b->p = (char*)realloc(b->p, cap);
    b->cap = cap;
}

void sr_putn(SRBuf* b, const char* s, int n)
{
    sr_grow(b, n);
    memcpy(b->p + b->len, s, n);
    b->len += n;
    b->p[b->len] = 0;
}

void sr_put(SRBuf* b, const char* s) { sr_putn(b, s, (int)strlen(s)); }

void sr_putf(SRBuf* b, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    sr_grow(b, n);
    va_start(ap, fmt);
    vsnprintf(b->p + b->len, n + 1, fmt, ap);
    va_end(ap);
    b->len += n;
}

/* ------------------------------------------------------------------------- */
/* hash map: 64-bit key -> pointer-sized value, open addressing              */

typedef struct SRMap { uint64_t* keys; uintptr_t* vals; unsigned cap, n; } SRMap;

static unsigned sr_hash(uint64_t k)
{
    k ^= k >> 33; k *= 0xff51afd7ed558ccdULL; k ^= k >> 33; k *= 0xc4ceb9fe1a85ec53ULL; k ^= k >> 33;
    return (unsigned)k;
}

static int sr_map_find(SRMap* m, uint64_t k, uintptr_t* v)
{
    if (!m->cap) return 0;
    for (unsigned i = sr_hash(k) & (m->cap - 1);; i = (i + 1) & (m->cap - 1)) {
        if (m->keys[i] == 0) return 0;
        if (m->keys[i] == k) { if (v) *v = m->vals[i]; return 1; }
    }
}

static void sr_map_put(SRMap* m, uint64_t k, uintptr_t v);

static void sr_map_rehash(SRMap* m, unsigned cap)
{
    SRMap old = *m;
    m->keys = (uint64_t*)calloc(cap, sizeof(uint64_t));
    m->vals = (uintptr_t*)calloc(cap, sizeof(uintptr_t));
    m->cap = cap;
    m->n = 0;
    for (unsigned i = 0; i < old.cap; i++)
        if (old.keys[i] && old.keys[i] != ~0ULL) sr_map_put(m, old.keys[i], old.vals[i]);
    free(old.keys);
    free(old.vals);
}

/* keys are never 0; ~0 marks a deleted slot */
static void sr_map_put(SRMap* m, uint64_t k, uintptr_t v)
{
    if ((m->n + 1) * 2 > m->cap) sr_map_rehash(m, m->cap ? m->cap * 2 : 1024);
    unsigned i = sr_hash(k) & (m->cap - 1);
    for (;; i = (i + 1) & (m->cap - 1)) {
        if (m->keys[i] == k) { m->vals[i] = v; return; }
        if (m->keys[i] == 0) break;
    }
    m->keys[i] = k;
    m->vals[i] = v;
    m->n++;
}

static void sr_map_del(SRMap* m, uint64_t k)
{
    /* linear probing delete with backward-shift */
    if (!m->cap) return;
    unsigned i = sr_hash(k) & (m->cap - 1);
    for (;; i = (i + 1) & (m->cap - 1)) {
        if (m->keys[i] == 0) return;
        if (m->keys[i] == k) break;
    }
    unsigned j = i;
    for (;;) {
        j = (j + 1) & (m->cap - 1);
        if (m->keys[j] == 0) break;
        unsigned h = sr_hash(m->keys[j]) & (m->cap - 1);
        if ((j > i && (h <= i || h > j)) || (j < i && (h <= i && h > j))) {
            m->keys[i] = m->keys[j];
            m->vals[i] = m->vals[j];
            i = j;
        }
    }
    m->keys[i] = 0;
    m->vals[i] = 0;
    m->n--;
}

/* ------------------------------------------------------------------------- */
/* process state                                                             */

static CRITICAL_SECTION g_lock;
static DWORD g_main_tid;
static uintptr_t g_exe_lo, g_exe_hi;
static HANDLE g_log = INVALID_HANDLE_VALUE, g_warn = INVALID_HANDLE_VALUE;
static SRBuf g_out;                 /* pending log bytes */
static unsigned g_seq, g_top_seq;
static int g_depth;
static int g_shutdown;
static unsigned g_foreign_calls, g_offthread_calls;

/* line slots for the current top-level call (pre-order output) */
typedef struct SRLine { char* text; int depth; } SRLine;
static SRLine* g_lines;
static int g_nlines, g_caplines;

static SRMap g_wrap_by_real;   /* real ptr -> SRWrap* (primary) */
static SRMap g_wrap_by_pair;   /* real ptr ^ (iface<<32) -> SRWrap* (incompatible extra types) */
static SRMap g_is_wrap;        /* SRWrap* -> 1 */
static SRMap g_addr_ids;       /* ptr -> id */
static SRMap g_handle_ids;     /* handle -> id */
static SRMap g_closures;       /* (fn ^ ctx*k) -> SRClosure* chain head */
static unsigned g_next_obj = 1, g_next_addr = 1, g_next_handle = 1;

unsigned sr_top_seq(void) { return g_top_seq; }

static int sr_in_exe(void* ra) { return (uintptr_t)ra >= g_exe_lo && (uintptr_t)ra < g_exe_hi; }

static HANDLE sr_open_out(const char* env)
{
    char path[1024];
    DWORD n = GetEnvironmentVariableA(env, path, sizeof path);
    if (!n || n >= sizeof path) return INVALID_HANDLE_VALUE;
    return CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
}

static void sr_write_raw(HANDLE h, const char* p, int n)
{
    DWORD w;
    if (h != INVALID_HANDLE_VALUE && n > 0) WriteFile(h, p, n, &w, NULL);
}

void sr_flush(void)
{
    sr_write_raw(g_log, g_out.p, g_out.len);
    g_out.len = 0;
}

static void sr_emit_line(const char* text, int depth)
{
    for (int i = 0; i < depth; i++) sr_putn(&g_out, "  ", 2);
    sr_put(&g_out, text);
    sr_putn(&g_out, "\n", 1);
    if (g_out.len > (1 << 20)) sr_flush();
}

static void sr_flush_lines(void)
{
    for (int i = 0; i < g_nlines; i++) {
        sr_emit_line(g_lines[i].text ? g_lines[i].text : "?? (unfinished call)", g_lines[i].depth);
        free(g_lines[i].text);
    }
    g_nlines = 0;
}

static int sr_reserve_line(int depth)
{
    if (g_nlines == g_caplines) {
        g_caplines = g_caplines ? g_caplines * 2 : 256;
        g_lines = (SRLine*)realloc(g_lines, g_caplines * sizeof(SRLine));
    }
    g_lines[g_nlines].text = NULL;
    g_lines[g_nlines].depth = depth;
    return g_nlines++;
}

void sr_log_warn(const char* fmt, ...)
{
    if (GetCurrentThreadId() != g_main_tid) return;
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    char line[1100];
    snprintf(line, sizeof line, "!! %s", tmp);
    if (g_depth > 0) {
        int i = sr_reserve_line(g_depth);
        g_lines[i].text = _strdup(line);
    } else {
        sr_emit_line(line, 0);
    }
    sr_side_warn("%s", tmp);
}

void sr_side_warn(const char* fmt, ...)
{
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp - 1, fmt, ap);
    va_end(ap);
    if (n < 0 || n > (int)sizeof tmp - 2) n = sizeof tmp - 2;
    tmp[n++] = '\n';
    sr_write_raw(g_warn, tmp, n);
}

/* ------------------------------------------------------------------------- */
/* call gate                                                                 */

static void sr_begin_common(SRCall* c)
{
    sr_buf_init(&c->b);
    c->line = -1;
    c->log = 0;
    if (!c->game || g_shutdown) return;
    if (GetCurrentThreadId() != g_main_tid) {
        if (g_offthread_calls++ < 20) sr_side_warn("off-main-thread game call (not logged)");
        return;
    }
    if (sr_suppress) return;
    c->log = 1;
    c->seq = ++g_seq;
    if (g_depth == 0) g_top_seq = c->seq;
    c->depth = g_depth++;
    c->line = sr_reserve_line(c->depth);
}

void sr_begin(SRCall* c, void* ra, const char* iface, const char* method)
{
    c->game = sr_in_exe(ra);
    if (!c->game && g_foreign_calls++ < 50)
        sr_side_warn("foreign caller %p for %s::%s (passed through, not logged)", ra, iface, method);
    sr_begin_common(c);
    if (c->log) {
        if (iface) sr_putf(&c->b, "%s::%s(", iface, method);
        else sr_putf(&c->b, "%s(", method);
    }
}

void sr_begin_cb(SRCall* c, const char* name)
{
    c->game = 1;
    sr_begin_common(c);
    if (c->log) sr_putf(&c->b, "<- %s(", name);
}

void sr_arg_sep(SRCall* c) { sr_putn(&c->b, ", ", 2); }

void sr_end(SRCall* c)
{
    if (!c->log) { sr_buf_free(&c->b); return; }
    g_lines[c->line].text = c->b.p ? c->b.p : _strdup("");
    g_depth--;
    if (g_depth == 0) {
        sr_deliver_messages(c->seq);
        if (g_depth == 0) sr_flush_lines();
    }
}

/* ------------------------------------------------------------------------- */
/* wrapper identity                                                          */

SRWrap* sr_as_wrap(void* p)
{
    if (!p) return NULL;
    uintptr_t v;
    EnterCriticalSection(&g_lock);
    int f = sr_map_find(&g_is_wrap, (uint64_t)(uintptr_t)p, &v);
    LeaveCriticalSection(&g_lock);
    return f ? (SRWrap*)p : NULL;
}

void* sr_unwrap(void* p)
{
    SRWrap* w = sr_as_wrap(p);
    return w ? w->real : p;
}

static int g_rmobject = -2; /* iface index of IDirect3DRMObject */

static void sr_retire_locked(SRWrap* w)
{
    uintptr_t v;
    if (sr_map_find(&g_wrap_by_real, (uint64_t)(uintptr_t)w->real, &v) && (SRWrap*)v == w)
        sr_map_del(&g_wrap_by_real, (uint64_t)(uintptr_t)w->real);
    uint64_t k = (uint64_t)(uintptr_t)w->real ^ ((uint64_t)(w->iface + 1) << 32);
    if (sr_map_find(&g_wrap_by_pair, k, &v) && (SRWrap*)v == w) sr_map_del(&g_wrap_by_pair, k);
}

/* The wrapper stays a valid forwarding object (the game may still hold the
 * pointer); only the real-address -> wrapper mapping ends. */
static void sr_retire(SRWrap* w)
{
    EnterCriticalSection(&g_lock);
    sr_retire_locked(w);
    LeaveCriticalSection(&g_lock);
}

static void __cdecl sr_destroy_cb(IDirect3DRMObject* obj, void* arg)
{
    (void)obj;
    sr_retire((SRWrap*)arg);
}

static SRWrap* sr_new_wrap(void* real, int iface)
{
    SRWrap* w = (SRWrap*)calloc(1, sizeof(SRWrap));
    w->lpVtbl = sr_ifaces[iface].vtbl;
    w->real = real;
    w->iface = iface;
    w->id = g_next_obj++;
    sr_map_put(&g_is_wrap, (uint64_t)(uintptr_t)w, 1);
    if (g_rmobject == -2) g_rmobject = sr_iface_by_name("IDirect3DRMObject");
    if (iface == g_rmobject || sr_ifaces[iface].ancestors[g_rmobject]) {
        IDirect3DRMObject* o = (IDirect3DRMObject*)real;
        w->d3drm = SUCCEEDED(o->lpVtbl->AddDestroyCallback(o, sr_destroy_cb, w));
    }
    return w;
}

static void* sr_wrap_common(void* real, int iface, int acquire)
{
    if (!real) return NULL;
    SRWrap* already = sr_as_wrap(real);
    if (already) { /* the game passed one of ours back out */
        if (acquire) already->grefs++;
        return real;
    }
    EnterCriticalSection(&g_lock);
    uintptr_t v;
    SRWrap* w = NULL;
    if (sr_map_find(&g_wrap_by_real, (uint64_t)(uintptr_t)real, &v)) {
        w = (SRWrap*)v;
        if (w->iface == iface || sr_ifaces[w->iface].ancestors[iface]) {
            /* existing wrapper's vtable already covers the requested one */
        } else if (sr_ifaces[iface].ancestors[w->iface]) {
            /* a more derived view of the same pointer: upgrade in place */
            w->lpVtbl = sr_ifaces[iface].vtbl;
            w->iface = iface;
        } else {
            uint64_t k = (uint64_t)(uintptr_t)real ^ ((uint64_t)(iface + 1) << 32);
            if (sr_map_find(&g_wrap_by_pair, k, &v)) {
                w = (SRWrap*)v;
            } else {
                w = sr_new_wrap(real, iface);
                sr_map_put(&g_wrap_by_pair, k, (uintptr_t)w);
            }
        }
    } else {
        w = sr_new_wrap(real, iface);
        sr_map_put(&g_wrap_by_real, (uint64_t)(uintptr_t)real, (uintptr_t)w);
    }
    if (acquire) w->grefs++;
    LeaveCriticalSection(&g_lock);
    return w;
}

void* sr_wrap(void* real, int iface) { return sr_wrap_common(real, iface, 0); }
void* sr_wrap_out(void* real, int iface) { return sr_wrap_common(real, iface, 1); }

static void* sr_wrap_iid_common(void* real, const IID* iid, int acquire)
{
    if (!real) return NULL;
    int i = sr_iface_by_iid(iid);
    if (i < 0) {
        SRBuf b;
        sr_buf_init(&b);
        sr_f_guid(&b, iid);
        sr_log_warn("UNWRAPPED interface %s handed to the game as a real pointer (calls on it are not logged)", b.p);
        sr_buf_free(&b);
        return real;
    }
    return sr_wrap_common(real, i, acquire);
}

void* sr_wrap_iid(void* real, const IID* iid) { return sr_wrap_iid_common(real, iid, 0); }
void* sr_wrap_iid_out(void* real, const IID* iid) { return sr_wrap_iid_common(real, iid, 1); }

void sr_on_release(SRWrap* w, ULONG ret)
{
    w->grefs--;
    if (ret == 0 || (!w->d3drm && w->grefs <= 0)) sr_retire(w);
}

int sr_iface_by_iid(const IID* iid)
{
    if (!iid) return -1;
    for (int i = 0; i < sr_niface; i++)
        if (sr_ifaces[i].iid && IsEqualGUID(sr_ifaces[i].iid, iid)) return i;
    return -1;
}

int sr_iface_by_name(const char* name)
{
    for (int i = 0; i < sr_niface; i++)
        if (!strcmp(sr_ifaces[i].name, name)) return i;
    return -1;
}

/* ------------------------------------------------------------------------- */
/* closures                                                                  */

typedef struct SRClosureNode { SRClosure c; struct SRClosureNode* next; } SRClosureNode;

SRClosure* sr_closure(void* fn, void* ctx, int kind)
{
    uint64_t k = ((uint64_t)(uintptr_t)fn << 32) ^ (uint64_t)(uintptr_t)ctx ^ ((uint64_t)kind << 56) ^ 1;
    EnterCriticalSection(&g_lock);
    uintptr_t v = 0;
    sr_map_find(&g_closures, k, &v);
    SRClosureNode* n;
    for (n = (SRClosureNode*)v; n; n = n->next)
        if (n->c.fn == fn && n->c.ctx == ctx && n->c.kind == kind) break;
    if (!n) {
        n = (SRClosureNode*)calloc(1, sizeof *n);
        n->c.fn = fn;
        n->c.ctx = ctx;
        n->c.kind = kind;
        n->next = (SRClosureNode*)v;
        sr_map_put(&g_closures, k, (uintptr_t)n);
    }
    LeaveCriticalSection(&g_lock);
    return &n->c;
}

/* ------------------------------------------------------------------------- */
/* formatting                                                                */

static unsigned sr_id(SRMap* m, unsigned* next, const void* p)
{
    uintptr_t v;
    EnterCriticalSection(&g_lock);
    if (!sr_map_find(m, (uint64_t)(uintptr_t)p, &v)) {
        v = (*next)++;
        sr_map_put(m, (uint64_t)(uintptr_t)p, v);
    }
    LeaveCriticalSection(&g_lock);
    return (unsigned)v;
}

unsigned sr_addr_id(const void* p) { return p ? sr_id(&g_addr_ids, &g_next_addr, p) : 0; }

void sr_f_u32(SRBuf* b, unsigned v) { sr_putf(b, "%u", v); }
void sr_f_i32(SRBuf* b, int v) { sr_putf(b, "%d", v); }
void sr_f_hex(SRBuf* b, unsigned v) { sr_putf(b, "0x%08x", v); }
void sr_f_i64(SRBuf* b, long long v) { sr_putf(b, "%lld", v); }
void sr_f_hr(SRBuf* b, HRESULT v) { sr_putf(b, "0x%08lx", (unsigned long)v); }

void sr_f_f32(SRBuf* b, float v)
{
    uint32_t bits;
    memcpy(&bits, &v, 4);
    if (v != v || bits == 0x7f800000u || bits == 0xff800000u) sr_putf(b, "f32:%08x", bits);
    else sr_putf(b, "%.9g", (double)v);
}

void sr_f_f64(SRBuf* b, double v)
{
    if (v != v) sr_put(b, "nan");
    else sr_putf(b, "%.17g", v);
}

void sr_f_obj(SRBuf* b, const void* p)
{
    if (!p) { sr_put(b, "NULL"); return; }
    SRWrap* w = sr_as_wrap((void*)p);
    if (w) sr_putf(b, "#%u", w->id);
    else sr_putf(b, "real@%u", sr_addr_id(p));
}

void sr_f_ptr(SRBuf* b, const void* p)
{
    if (!p) sr_put(b, "NULL");
    else if (sr_in_exe((void*)p)) sr_putf(b, "exe:%08x", (unsigned)(uintptr_t)p);
    else if (sr_as_wrap((void*)p)) sr_f_obj(b, p);
    else sr_putf(b, "@%u", sr_addr_id(p));
}

void sr_f_fn(SRBuf* b, const void* fn) { sr_f_ptr(b, fn); }

void sr_f_handle(SRBuf* b, const void* h)
{
    if (!h) sr_put(b, "0");
    else if (h == INVALID_HANDLE_VALUE) sr_put(b, "INVALID");
    else sr_putf(b, "h%u", sr_id(&g_handle_ids, &g_next_handle, h));
}

void sr_handle_new(const void* h)
{
    if (!h || h == INVALID_HANDLE_VALUE) return;
    EnterCriticalSection(&g_lock);
    sr_map_put(&g_handle_ids, (uint64_t)(uintptr_t)h, g_next_handle++);
    LeaveCriticalSection(&g_lock);
}

/* The game builds absolute paths from its own directory: print them relative
 * to it, so logs do not depend on where the game was staged. */
static char g_gamedir[2][MAX_PATH]; /* exe directory, current directory */
static int g_gamedir_len[2];

static int sr_gamedir_prefix(const char* s)
{
    for (int i = 0; i < 2; i++)
        if (g_gamedir_len[i] && !_strnicmp(s, g_gamedir[i], g_gamedir_len[i])) return g_gamedir_len[i];
    return 0;
}

void sr_f_str(SRBuf* b, const char* s)
{
    if (!s) { sr_put(b, "NULL"); return; }
    sr_putn(b, "\"", 1);
    int skip = sr_gamedir_prefix(s);
    if (skip) { sr_put(b, "<game>\\\\"); s += skip; }
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == '"' || ch == '\\') sr_putf(b, "\\%c", ch);
        else if (ch < 0x20 || ch >= 0x7f) sr_putf(b, "\\x%02x", ch);
        else sr_putn(b, (const char*)&ch, 1);
    }
    sr_putn(b, "\"", 1);
}

void sr_f_wstr(SRBuf* b, const WCHAR* s)
{
    if (!s) { sr_put(b, "NULL"); return; }
    sr_put(b, "L\"");
    for (int d = 0; d < 2; d++) {
        int i = 0;
        if (!g_gamedir_len[d]) continue;
        while (i < g_gamedir_len[d] && s[i] && towlower(s[i]) == (WCHAR)tolower((unsigned char)g_gamedir[d][i])) i++;
        if (i == g_gamedir_len[d]) { sr_put(b, "<game>\\\\"); s += i; break; }
    }
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') sr_putf(b, "\\%c", (char)*s);
        else if (*s < 0x20 || *s >= 0x7f) sr_putf(b, "\\u%04x", *s);
        else { char c = (char)*s; sr_putn(b, &c, 1); }
    }
    sr_put(b, "\"");
}

/* Names for the GUIDs the game passes that are not interface IIDs. */
static const struct { const GUID* g; const char* name; } sr_guid_names[] = {
    { &IID_IDirect3DRGBDevice, "IID_IDirect3DRGBDevice" },
    { &IID_IDirect3DHALDevice, "IID_IDirect3DHALDevice" },
    { &IID_IDirect3DMMXDevice, "IID_IDirect3DMMXDevice" },
    { &IID_IDirect3DRampDevice, "IID_IDirect3DRampDevice" },
    { &IID_IDirect3DRefDevice, "IID_IDirect3DRefDevice" },
    { &GUID_SysKeyboard, "GUID_SysKeyboard" },
    { &GUID_SysMouse, "GUID_SysMouse" },
    { &CLSID_AMMultiMediaStream, "CLSID_AMMultiMediaStream" },
    { &MSPID_PrimaryVideo, "MSPID_PrimaryVideo" },
    { &MSPID_PrimaryAudio, "MSPID_PrimaryAudio" },
};

void sr_f_guid(SRBuf* b, const GUID* g)
{
    if (!g) { sr_put(b, "NULL"); return; }
    for (int i = 0; i < sr_niface; i++)
        if (sr_ifaces[i].iid && IsEqualGUID(sr_ifaces[i].iid, g)) { sr_putf(b, "IID_%s", sr_ifaces[i].name); return; }
    for (unsigned i = 0; i < sizeof sr_guid_names / sizeof sr_guid_names[0]; i++)
        if (IsEqualGUID(sr_guid_names[i].g, g)) { sr_put(b, sr_guid_names[i].name); return; }
    sr_putf(b, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", (unsigned long)g->Data1, g->Data2,
            g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3], g->Data4[4], g->Data4[5],
            g->Data4[6], g->Data4[7]);
}

void sr_f_bytes(SRBuf* b, const void* p, unsigned n)
{
    const unsigned char* q = (const unsigned char*)p;
    sr_grow(b, n * 2);
    for (unsigned i = 0; i < n; i++) sr_putf(b, "%02x", q[i]);
}

void sr_f_floats(SRBuf* b, const float* p, unsigned n)
{
    sr_putn(b, "[", 1);
    for (unsigned i = 0; i < n; i++) {
        if (i) sr_putn(b, " ", 1);
        sr_f_f32(b, p[i]);
    }
    sr_putn(b, "]", 1);
}

static uint32_t sr_fnv(const void* p, unsigned n)
{
    const unsigned char* q = (const unsigned char*)p;
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < n; i++) h = (h ^ q[i]) * 16777619u;
    return h;
}

/* -- struct formatters ------------------------------------------------------ */

static void fx_rect(SRBuf* b, const void* p)
{
    const RECT* r = (const RECT*)p;
    sr_putf(b, "{%ld,%ld,%ld,%ld}", r->left, r->top, r->right, r->bottom);
}

static void fx_point(SRBuf* b, const void* p)
{
    const POINT* r = (const POINT*)p;
    sr_putf(b, "{%ld,%ld}", r->x, r->y);
}

static void fx_vec(SRBuf* b, const void* p) { sr_f_floats(b, (const float*)p, 3); }
static void fx_box(SRBuf* b, const void* p) { sr_f_floats(b, (const float*)p, 6); }
static void fx_quat(SRBuf* b, const void* p) { sr_f_floats(b, (const float*)p, 4); }
static void fx_mat4(SRBuf* b, const void* p) { sr_f_floats(b, (const float*)p, 16); }

static void fx_colorkey(SRBuf* b, const void* p)
{
    const DDCOLORKEY* k = (const DDCOLORKEY*)p;
    sr_putf(b, "{0x%lx,0x%lx}", k->dwColorSpaceLowValue, k->dwColorSpaceHighValue);
}

static void fx_pixfmt(SRBuf* b, const void* p)
{
    const DDPIXELFORMAT* f = (const DDPIXELFORMAT*)p;
    sr_putf(b, "{size=%lu flags=0x%lx fourcc=0x%lx bits=%lu masks=0x%lx/0x%lx/0x%lx/0x%lx}", f->dwSize,
            f->dwFlags, f->dwFourCC, f->dwRGBBitCount, f->dwRBitMask, f->dwGBitMask, f->dwBBitMask,
            f->dwRGBAlphaBitMask);
}

static void fx_sd_common(SRBuf* b, DWORD size, DWORD flags, DWORD h, DWORD w, LONG pitch, DWORD count,
                         DWORD zbits, DWORD refresh, DWORD alpha, const void* surf, const DDCOLORKEY* ck,
                         const DDPIXELFORMAT* pf, DWORD caps, DWORD caps2)
{
    sr_putf(b, "{size=%lu flags=0x%lx", size, flags);
    if (flags & DDSD_HEIGHT) sr_putf(b, " h=%lu", h);
    if (flags & DDSD_WIDTH) sr_putf(b, " w=%lu", w);
    if (flags & DDSD_PITCH) sr_putf(b, " pitch=%ld", pitch);
    if (flags & DDSD_BACKBUFFERCOUNT) sr_putf(b, " backbuffers=%lu", count);
    if (flags & DDSD_MIPMAPCOUNT) sr_putf(b, " mips=%lu", count);
    if (flags & DDSD_ZBUFFERBITDEPTH) sr_putf(b, " zbits=%lu", zbits);
    if (flags & DDSD_REFRESHRATE) sr_putf(b, " refresh=%lu", refresh);
    if (flags & DDSD_ALPHABITDEPTH) sr_putf(b, " alphabits=%lu", alpha);
    if (flags & DDSD_LPSURFACE) { sr_put(b, " surface="); sr_f_ptr(b, surf); }
    if (flags & DDSD_CKDESTBLT) { sr_put(b, " ckdestblt="); fx_colorkey(b, &ck[1]); }
    if (flags & DDSD_CKSRCBLT) { sr_put(b, " cksrcblt="); fx_colorkey(b, &ck[3]); }
    if (flags & DDSD_PIXELFORMAT) { sr_put(b, " pf="); fx_pixfmt(b, pf); }
    if (flags & DDSD_CAPS) sr_putf(b, " caps=0x%lx/0x%lx", caps, caps2);
    sr_put(b, "}");
}

static void fx_sd(SRBuf* b, const void* p)
{
    const DDSURFACEDESC* d = (const DDSURFACEDESC*)p;
    fx_sd_common(b, d->dwSize, d->dwFlags, d->dwHeight, d->dwWidth, d->lPitch, d->dwBackBufferCount,
                 d->dwZBufferBitDepth, d->dwRefreshRate, d->dwAlphaBitDepth, d->lpSurface,
                 &d->ddckCKDestOverlay, &d->ddpfPixelFormat, d->ddsCaps.dwCaps, 0);
}

static void fx_sd2(SRBuf* b, const void* p)
{
    const DDSURFACEDESC2* d = (const DDSURFACEDESC2*)p;
    fx_sd_common(b, d->dwSize, d->dwFlags, d->dwHeight, d->dwWidth, d->lPitch, d->dwBackBufferCount,
                 0, d->dwRefreshRate, d->dwAlphaBitDepth, d->lpSurface, &d->ddckCKDestOverlay,
                 &d->ddpfPixelFormat, d->ddsCaps.dwCaps, d->ddsCaps.dwCaps2);
}

static void fx_caps(SRBuf* b, const void* p) { sr_putf(b, "{0x%lx}", ((const DDSCAPS*)p)->dwCaps); }
static void fx_caps2(SRBuf* b, const void* p)
{
    const DDSCAPS2* c = (const DDSCAPS2*)p;
    sr_putf(b, "{0x%lx,0x%lx,0x%lx,0x%lx}", c->dwCaps, c->dwCaps2, c->dwCaps3, c->dwCaps4);
}

static void fx_bltfx(SRBuf* b, const void* p)
{
    const DDBLTFX* f = (const DDBLTFX*)p;
    sr_putf(b, "{size=%lu fx=0x%lx rop=0x%lx fill=0x%lx ckdest=", f->dwSize, f->dwDDFX, f->dwROP,
            f->dwFillColor);
    fx_colorkey(b, &f->ddckDestColorkey);
    sr_put(b, " cksrc=");
    fx_colorkey(b, &f->ddckSrcColorkey);
    sr_put(b, "}");
}

static void fx_wfx(SRBuf* b, const void* p)
{
    const WAVEFORMATEX* w = (const WAVEFORMATEX*)p;
    sr_putf(b, "{tag=%u ch=%u rate=%lu avg=%lu align=%u bits=%u}", w->wFormatTag, w->nChannels,
            w->nSamplesPerSec, w->nAvgBytesPerSec, w->nBlockAlign, w->wBitsPerSample);
}

static void fx_dsbd(SRBuf* b, const void* p)
{
    const DSBUFFERDESC* d = (const DSBUFFERDESC*)p;
    sr_putf(b, "{size=%lu flags=0x%lx bytes=%lu fmt=", d->dwSize, d->dwFlags, d->dwBufferBytes);
    if (d->lpwfxFormat) fx_wfx(b, d->lpwfxFormat);
    else sr_put(b, "NULL");
    sr_put(b, "}");
}

static void fx_msg(SRBuf* b, const void* p)
{
    const MSG* m = (const MSG*)p;
    sr_put(b, "{");
    sr_f_handle(b, m->hwnd);
    sr_putf(b, " 0x%04x %u 0x%lx}", m->message, (unsigned)m->wParam, (unsigned long)m->lParam);
}

static void fx_wndclass(SRBuf* b, const void* p)
{
    const WNDCLASSA* w = (const WNDCLASSA*)p;
    sr_putf(b, "{style=0x%x proc=", w->style);
    sr_f_fn(b, (const void*)w->lpfnWndProc);
    sr_put(b, " inst=");
    sr_f_handle(b, w->hInstance);
    sr_put(b, " class=");
    sr_f_str(b, w->lpszClassName);
    sr_put(b, "}");
}

static void fx_d3drmimage(SRBuf* b, const void* p)
{
    const D3DRMIMAGE* i = (const D3DRMIMAGE*)p;
    sr_putf(b, "{w=%d h=%d aspect=%d/%d depth=%d rgb=%d pitch=%d buf=", i->width, i->height, i->aspectx,
            i->aspecty, i->depth, i->rgb, i->bytes_per_line);
    sr_f_ptr(b, i->buffer1);
    if (i->buffer1 && i->bytes_per_line > 0 && i->height > 0 && i->height < 4096)
        sr_putf(b, " fnv=%08x", sr_fnv(i->buffer1, (unsigned)(i->bytes_per_line * i->height)));
    sr_putf(b, " masks=0x%lx/0x%lx/0x%lx/0x%lx pal=%d}", i->red_mask, i->green_mask, i->blue_mask,
            i->alpha_mask, i->palette_size);
}

static void fx_mmckinfo(SRBuf* b, const void* p)
{
    const MMCKINFO* c = (const MMCKINFO*)p;
    sr_putf(b, "{id=%.4s size=%lu type=%.4s off=%lu flags=0x%lx}", (const char*)&c->ckid, c->cksize,
            (const char*)&c->fccType, c->dwDataOffset, c->dwFlags);
}

static void fx_mmioinfo(SRBuf* b, const void* p)
{
    const MMIOINFO* m = (const MMIOINFO*)p;
    sr_putf(b, "{flags=0x%lx buf=%ld next=%ld endread=%ld endwrite=%ld disk=%ld}", m->dwFlags,
            m->cchBuffer, m->pchNext && m->pchBuffer ? (long)(m->pchNext - m->pchBuffer) : -1L,
            m->pchEndRead && m->pchBuffer ? (long)(m->pchEndRead - m->pchBuffer) : -1L,
            m->pchEndWrite && m->pchBuffer ? (long)(m->pchEndWrite - m->pchBuffer) : -1L, m->lDiskOffset);
}

static void fx_d3dvp2(SRBuf* b, const void* p)
{
    const D3DVIEWPORT2* v = (const D3DVIEWPORT2*)p;
    sr_putf(b, "{size=%lu x=%lu y=%lu w=%lu h=%lu clip=", v->dwSize, v->dwX, v->dwY, v->dwWidth, v->dwHeight);
    sr_f_floats(b, &v->dvClipX, 6);
    sr_put(b, "}");
}

static void fx_mxlc(SRBuf* b, const void* p)
{
    const MIXERLINECONTROLSA* m = (const MIXERLINECONTROLSA*)p;
    sr_putf(b, "{size=%lu line=%lu ctl=0x%lx n=%lu cb=%lu ctls=", m->cbStruct, m->dwLineID, m->dwControlID,
            m->cControls, m->cbmxctrl);
    if (m->pamxctrl && m->cControls && m->cbmxctrl && m->cControls * m->cbmxctrl < 65536)
        sr_putf(b, "fnv:%08x}", sr_fnv(m->pamxctrl, m->cControls * m->cbmxctrl));
    else
        sr_put(b, "NULL}");
}

static void fx_mxcd(SRBuf* b, const void* p)
{
    const MIXERCONTROLDETAILS* m = (const MIXERCONTROLDETAILS*)p;
    sr_putf(b, "{size=%lu ctl=%lu ch=%lu items=%lu cb=%lu details=", m->cbStruct, m->dwControlID, m->cChannels,
            m->cMultipleItems, m->cbDetails);
    unsigned n = m->cChannels * (m->cMultipleItems ? m->cMultipleItems : 1) * m->cbDetails;
    if (m->paDetails && n && n <= 256) sr_f_bytes(b, m->paDetails, n);
    else sr_put(b, "NULL");
    sr_put(b, "}");
}

static void fx_mxcaps(SRBuf* b, const void* p)
{
    const MIXERCAPSA* m = (const MIXERCAPSA*)p;
    char name[MAXPNAMELEN + 1];
    memcpy(name, m->szPname, MAXPNAMELEN);
    name[MAXPNAMELEN] = 0;
    sr_putf(b, "{mid=%u pid=%u ver=0x%x name=", m->wMid, m->wPid, m->vDriverVersion);
    sr_f_str(b, name);
    sr_putf(b, " support=0x%lx dests=%lu}", m->fdwSupport, m->cDestinations);
}

static void fx_mxline(SRBuf* b, const void* p)
{
    const MIXERLINEA* m = (const MIXERLINEA*)p;
    char sname[MIXER_SHORT_NAME_CHARS + 1], name[MIXER_LONG_NAME_CHARS + 1], tname[MAXPNAMELEN + 1];
    memcpy(sname, m->szShortName, MIXER_SHORT_NAME_CHARS);
    sname[MIXER_SHORT_NAME_CHARS] = 0;
    memcpy(name, m->szName, MIXER_LONG_NAME_CHARS);
    name[MIXER_LONG_NAME_CHARS] = 0;
    memcpy(tname, m->Target.szPname, MAXPNAMELEN);
    tname[MAXPNAMELEN] = 0;
    sr_putf(b, "{size=%lu dest=%lu src=%lu line=0x%lx status=0x%lx type=0x%lx ch=%lu conn=%lu ctls=%lu short=",
            m->cbStruct, m->dwDestination, m->dwSource, m->dwLineID, m->fdwLine, m->dwComponentType,
            m->cChannels, m->cConnections, m->cControls);
    sr_f_str(b, sname);
    sr_put(b, " name=");
    sr_f_str(b, name);
    sr_putf(b, " target={type=%lu dev=%lu mid=%u pid=%u ver=0x%x name=", m->Target.dwType, m->Target.dwDeviceID,
            m->Target.wMid, m->Target.wPid, m->Target.vDriverVersion);
    sr_f_str(b, tname);
    sr_put(b, "}}");
}

static const struct { const char* name; void (*fn)(SRBuf*, const void*); } sr_fx[] = {
    { "MIXERCAPSA", fx_mxcaps }, { "MIXERLINEA", fx_mxline },
    { "MIXERLINECONTROLSA", fx_mxlc }, { "MIXERCONTROLDETAILS", fx_mxcd },
    { "RECT", fx_rect }, { "POINT", fx_point }, { "D3DVECTOR", fx_vec }, { "D3DRMBOX", fx_box },
    { "D3DRMQUATERNION", fx_quat }, { "D3DRMMATRIX4D", fx_mat4 }, { "D3DMATRIX", fx_mat4 },
    { "DDCOLORKEY", fx_colorkey }, { "DDPIXELFORMAT", fx_pixfmt }, { "DDSURFACEDESC", fx_sd },
    { "DDSURFACEDESC2", fx_sd2 }, { "DDSCAPS", fx_caps }, { "DDSCAPS2", fx_caps2 }, { "DDBLTFX", fx_bltfx },
    { "WAVEFORMATEX", fx_wfx }, { "DSBUFFERDESC", fx_dsbd }, { "MSG", fx_msg }, { "WNDCLASSA", fx_wndclass },
    { "D3DRMIMAGE", fx_d3drmimage }, { "MMCKINFO", fx_mmckinfo }, { "MMIOINFO", fx_mmioinfo },
    { "D3DVIEWPORT2", fx_d3dvp2 },
};

void sr_f_struct(SRBuf* b, const char* type, const void* p, unsigned size)
{
    if (!p) { sr_put(b, "NULL"); return; }
    for (unsigned i = 0; i < sizeof sr_fx / sizeof sr_fx[0]; i++)
        if (!strcmp(sr_fx[i].name, type)) { sr_fx[i].fn(b, p); return; }
    if (size <= 64) {
        sr_put(b, "<");
        sr_f_bytes(b, p, size);
        sr_put(b, ">");
    } else {
        sr_putf(b, "<%u:fnv=%08x>", size, sr_fnv(p, size));
    }
}

/* ------------------------------------------------------------------------- */
/* real DLLs                                                                 */

static struct { const char* name; HMODULE h; } g_real[16];

SR_EXPORT HMODULE SR_LoadReal(const char* dll)
{
    for (int i = 0; i < 16; i++) {
        if (g_real[i].name && !_stricmp(g_real[i].name, dll)) return g_real[i].h;
        if (!g_real[i].name) {
            char path[MAX_PATH * 2];
            char env[64];
            snprintf(env, sizeof env, "SEAMREC_REAL_%s", dll);
            DWORD n = GetEnvironmentVariableA(env, path, sizeof path);
            if (!n || n >= sizeof path) {
                if (!_stricmp(dll, "d3drm")) {
                    /* Microsoft's D3DRM.DLL, staged beside the exe under another name */
                    GetModuleFileNameA(NULL, path, sizeof path);
                    char* slash = strrchr(path, '\\');
                    strcpy(slash ? slash + 1 : path, "d3drm_ms.dll");
                } else {
                    GetSystemDirectoryA(path, MAX_PATH);
                    strcat(path, "\\");
                    strcat(path, dll);
                    strcat(path, ".dll");
                }
            }
            HMODULE h = LoadLibraryA(path);
            if (!h) {
                sr_side_warn("FATAL: cannot load real %s from %s (error %lu)", dll, path, GetLastError());
                sr_flush();
                ExitProcess(97);
            }
            g_real[i].name = _strdup(dll);
            g_real[i].h = h;
            return h;
        }
    }
    return NULL;
}

FARPROC sr_real_proc(const char* dll, const char* name)
{
    FARPROC f = GetProcAddress(SR_LoadReal(dll), name);
    if (!f) {
        sr_side_warn("FATAL: real %s has no export %s", dll, name);
        sr_flush();
        ExitProcess(97);
    }
    return f;
}

/* ------------------------------------------------------------------------- */
/* lifecycle                                                                 */

void sr_install_iat(HMODULE exe); /* gen_flat.c */

void sr_shutdown(const char* why)
{
    if (g_shutdown) return;
    /* close any half-built top-level line group */
    while (g_depth > 0) g_depth--;
    sr_flush_lines();
    char tmp[256];
    snprintf(tmp, sizeof tmp, "-- end (%s) calls=%u frames=%u objects=%u", why, g_seq, sr_frame, g_next_obj - 1);
    sr_emit_line(tmp, 0);
    g_shutdown = 1;
    sr_scn_finish();
    sr_flush();
    sr_side_warn("summary: %s; foreign wrapper calls=%u off-thread game calls=%u", tmp, g_foreign_calls,
                 g_offthread_calls);
    if (g_log != INVALID_HANDLE_VALUE) CloseHandle(g_log);
    g_log = INVALID_HANDLE_VALUE;
}

static void sr_init(HMODULE self)
{
    (void)self;
    InitializeCriticalSection(&g_lock);
    g_main_tid = GetCurrentThreadId();
    HMODULE exe = GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((char*)exe + ((IMAGE_DOS_HEADER*)exe)->e_lfanew);
    g_exe_lo = (uintptr_t)exe;
    g_exe_hi = g_exe_lo + nt->OptionalHeader.SizeOfImage;
    GetModuleFileNameA(NULL, g_gamedir[0], MAX_PATH);
    char* slash = strrchr(g_gamedir[0], '\\');
    if (slash) { slash[1] = 0; g_gamedir_len[0] = (int)strlen(g_gamedir[0]); }
    DWORD n = GetCurrentDirectoryA(MAX_PATH - 1, g_gamedir[1]);
    if (n && n < MAX_PATH - 1) {
        if (g_gamedir[1][n - 1] != '\\') strcat(g_gamedir[1], "\\");
        g_gamedir_len[1] = (int)strlen(g_gamedir[1]);
    }
    char mode[32] = "";
    GetEnvironmentVariableA("SEAMREC_MODE", mode, sizeof mode);
    if (!_stricmp(mode, "record")) sr_mode = SR_RECORD;
    else if (!_stricmp(mode, "script")) sr_mode = SR_SCRIPT;
    else if (!_stricmp(mode, "replay")) sr_mode = SR_REPLAY;
    else sr_mode = SR_PASS;
    g_log = sr_open_out("SEAMREC_LOG");
    g_warn = sr_open_out("SEAMREC_WARN");
    sr_buf_init(&g_out);
    sr_put(&g_out, "seamrec-log 1\n");
    sr_side_warn("seamrec: mode=%s exe=%08x-%08x", mode[0] ? mode : "pass", (unsigned)g_exe_lo,
                 (unsigned)g_exe_hi);
    sr_scn_init();
    sr_install_iat(exe);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        sr_init(inst);
    } else if (reason == DLL_PROCESS_DETACH) {
        sr_shutdown("process detach");
    }
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* helpers used by generated code                                            */

int sr_is_game(void* ra) { return sr_in_exe(ra); }

/* Wrap an interface pointer handed out without an IID: probe which wrapped
 * interface answers QueryInterface with this very pointer, most derived wins. */
void* sr_wrap_identify(void* real)
{
    if (!real || sr_as_wrap(real)) return real;
    IUnknown* u = (IUnknown*)real;
    int best = -1;
    for (int i = 0; i < sr_niface; i++) {
        void* out = NULL;
        if (!sr_ifaces[i].iid) continue;
        if (SUCCEEDED(u->lpVtbl->QueryInterface(u, sr_ifaces[i].iid, &out)) && out) {
            if (out == real && (best < 0 || sr_ifaces[i].nmethods > sr_ifaces[best].nmethods)) best = i;
            ((IUnknown*)out)->lpVtbl->Release((IUnknown*)out);
        }
    }
    if (best < 0) {
        sr_log_warn("UNWRAPPED object of unknown interface handed to the game");
        return real;
    }
    return sr_wrap_out(real, best);
}

SR_EXPORT void SR_MissingExport(void)
{
    sr_side_warn("FATAL: a pass-through export missing from the real DLL was called");
    sr_shutdown("missing export");
    ExitProcess(96);
}
