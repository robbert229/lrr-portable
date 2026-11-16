/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Seam recorder runtime: shared by the generated wrappers (gen_*.c), the
 * hand-written hooks (hooks.c) and the core (core.c). See ../README.md. */
#ifndef SEAMREC_SR_H
#define SEAMREC_SR_H

#include "dxall.h"
#include <stdint.h>

#define SR_EXPORT __declspec(dllexport)
#define SR_RA() __builtin_return_address(0)

/* -- modes ------------------------------------------------------------------ */
enum { SR_PASS = 0, SR_RECORD = 1, SR_SCRIPT = 2, SR_REPLAY = 3 };
extern int sr_mode;

/* -- growable text buffer -------------------------------------------------- */
typedef struct SRBuf { char* p; int len, cap; } SRBuf;
void sr_buf_init(SRBuf* b);
void sr_buf_free(SRBuf* b);
void sr_put(SRBuf* b, const char* s);
void sr_putf(SRBuf* b, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void sr_putn(SRBuf* b, const char* s, int n);

/* -- interface table (generated) -------------------------------------------- */
typedef struct SRIface {
    const char* name;
    const IID* iid;           /* NULL when the headers have no IID for it */
    const void* vtbl;         /* the wrapper vtable */
    int nmethods;
    const unsigned char* ancestors; /* ancestors[j] != 0: iface j's vtable is a prefix of this one */
} SRIface;
extern const SRIface sr_ifaces[];
extern const int sr_niface;
int sr_iface_by_iid(const IID* iid); /* -1 when unknown */
int sr_iface_by_name(const char* name);

/* A wrapper object handed to the game in place of the real interface. */
typedef struct SRWrap {
    const void* lpVtbl;
    void* real;
    int iface;
    unsigned id;
    long grefs;       /* references the game holds through this wrapper */
    int d3drm;        /* identity ends with the D3DRM object's destroy callback */
} SRWrap;

/* Canonical wrapper for a real interface pointer (NULL -> NULL). The _out
 * forms are for pointers the game *acquires* (out-parameters: one reference
 * by COM rules); the plain forms for pointers it is merely shown (callback
 * arguments). Identity: a D3DRM object keeps its id until it is destroyed; any
 * other object until the game's own references drop to zero. A later sighting
 * of the same address after that is a new object with a new id, so ids never
 * depend on heap address reuse. */
void* sr_wrap(void* real, int iface);
void* sr_wrap_out(void* real, int iface);
void* sr_wrap_iid(void* real, const IID* iid); /* by IID; unknown IID -> real pointer + loud warning */
void* sr_wrap_iid_out(void* real, const IID* iid);
void sr_on_release(SRWrap* w, ULONG ret);
void* sr_unwrap(void* p);                      /* wrapper -> real, anything else unchanged */
SRWrap* sr_as_wrap(void* p);                   /* wrapper or NULL */

/* -- call gate ---------------------------------------------------------------
 * Every wrapper begins with sr_begin(&call, return address). call.game is 1
 * when the caller is LegoRR.exe (or a game callback); call.log is 1 when the
 * call is also to be logged (game, main thread, not inside a suppressed
 * system message). Wrapping of out-objects happens when call.game is set. */
typedef struct SRCall {
    int game, log, line, depth;
    unsigned seq;
    SRBuf b;
} SRCall;
void sr_begin(SRCall* c, void* ra, const char* iface, const char* method);
void sr_begin_cb(SRCall* c, const char* name);  /* callback from a real DLL into the game */
void sr_end(SRCall* c);                           /* finish the line (caller appended args + " = ret") */
void sr_arg_sep(SRCall* c);

/* -- value formatting (append to c->b, comma handled by caller via sr_arg_sep) */
void sr_f_u32(SRBuf* b, unsigned v);
void sr_f_i32(SRBuf* b, int v);
void sr_f_hex(SRBuf* b, unsigned v);
void sr_f_i64(SRBuf* b, long long v);
void sr_f_f32(SRBuf* b, float v);
void sr_f_f64(SRBuf* b, double v);
void sr_f_hr(SRBuf* b, HRESULT v);
void sr_f_obj(SRBuf* b, const void* p);       /* wrapper -> #id; NULL; else @addr-id */
void sr_f_ptr(SRBuf* b, const void* p);       /* opaque data pointer -> exe:VA | @id | NULL */
void sr_f_handle(SRBuf* b, const void* h);    /* handle -> h<id> */
void sr_handle_new(const void* h);             /* a creator returned h: give it a fresh id */
void sr_f_fn(SRBuf* b, const void* fn);       /* code pointer -> exe:VA or @id */
void sr_f_str(SRBuf* b, const char* s);
void sr_f_wstr(SRBuf* b, const WCHAR* s);
void sr_f_guid(SRBuf* b, const GUID* g);
void sr_f_bytes(SRBuf* b, const void* p, unsigned n);
void sr_f_floats(SRBuf* b, const float* p, unsigned n);
/* Struct pointer: a registered formatter by type name, else a byte dump. */
void sr_f_struct(SRBuf* b, const char* type, const void* p, unsigned size);
/* Opaque-pointer id without printing (for scenario keys). */
unsigned sr_addr_id(const void* p);

/* -- warnings: deterministic ones go into the seam log as "!! ..." lines,
 * process-level ones (foreign callers, threads) into the warnings file. */
void sr_log_warn(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void sr_side_warn(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

/* -- callbacks: closure lookup (by original fn+ctx) -------------------------- */
typedef struct SRClosure { void* fn; void* ctx; int kind; } SRClosure;
SRClosure* sr_closure(void* fn, void* ctx, int kind);

/* -- real DLLs ---------------------------------------------------------------- */
SR_EXPORT HMODULE SR_LoadReal(const char* dll);
FARPROC sr_real_proc(const char* dll, const char* name);

/* -- scenario / injection (hooks.c, scenario.c) ------------------------------ */
extern unsigned sr_frame;          /* number of game PeekMessageA pumps */
void sr_scn_init(void);
void sr_scn_finish(void);
int sr_scn_next(const char* chan, const char* key, char* out, int outsz); /* replay: 1 ok */
void sr_scn_write(const char* chan, const char* key, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
void sr_divergence(const char* what) __attribute__((noreturn));
void sr_pump_messages(void);       /* drain + dispatch the real queue (hooks.c) */
void sr_deliver_messages(unsigned seq);
void sr_on_frame(void);            /* advance virtual time, scripted events */
unsigned sr_top_seq(void);
extern int sr_suppress;
void sr_flush(void);
void sr_shutdown(const char* why);

/* Overrides hooks.c provides for specific generated wrappers. */
#include "gen_hooks.h"

#endif
