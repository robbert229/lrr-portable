#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Emit the seam recorder's C sources from the method tables.

    python3 gen.py OUTDIR

Inputs (next to this script):
  methods.txt      COM interfaces and callback typedefs (extract.py output)
  flat.txt         flat functions: proxy exports and IAT hooks
  exports/*.txt    the real DLLs' export tables, for the pass-through proxies

Outputs in OUTDIR:
  gen_wrappers.c   one logging wrapper per COM method, vtables, the interface
                   table, callback trampolines
  gen_flat.c       flat wrappers and the IAT installer
  gen_hooks.h      prototypes hooks.c implements, real-function pointers
  proxy_<dll>.c, <dll>.def  one pass-through proxy DLL per wrapped DLL

Every parameter is classified by its declared type (classify()); the kind
decides how the argument is passed on (wrappers unwrapped, callbacks routed
through trampolines) and how it is printed. Anything the classifier cannot
place safely is printed loudly ("!! ...") instead of silently passed.
"""
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent

# Methods whose real call goes through a hand-written hook in hooks.c.
HOOKS = {
    "IDirectInputDeviceA": {"GetDeviceState", "Acquire", "Unacquire"},
    "IDirectInputDevice2A": {"GetDeviceState", "Acquire", "Unacquire"},
    "IDirectSound": {"CreateSoundBuffer", "DuplicateSoundBuffer"},
    "IDirectSoundBuffer": {"GetCurrentPosition", "GetStatus", "Play", "Stop", "SetCurrentPosition",
                           "SetFrequency"},
    "IDirectDrawStreamSample": {"Update", "CompletionStatus"},
    "IStreamSample": {"Update", "CompletionStatus"},
    "IDirectDrawSurface": {"Flip"},
    "IDirectDrawSurface2": {"Flip"},
    "IDirectDrawSurface3": {"Flip"},
    "IDirectDrawSurface4": {"Flip"},
}

# Parameter overrides: (method, param) -> kind. "ptrval" prints an integer
# that carries a pointer (app data) as an opaque pointer id.
PARAM_KIND = {
    ("SetAppData", "data"): "ptrval",
    ("mmioRead", "pch"): "ptr",
    ("GetDeviceState", "lpvData"): "sparse",
}
# Count expressions for "sparse" parameters (nonzero bytes printed as idx:val).
SPARSE = {("GetDeviceState", "lpvData"): "cbData"}
RET_KIND = {"GetAppData": "ptrval"}

# Array parameters: (method, param) -> count expression (C, in terms of the
# other parameters, evaluated after the call).
COUNTS = {
    ("GetVisuals", "visuals"): "(count ? *count : 0)",
    ("RegQueryValueExA", "lpData"): "(lpcbData ? *lpcbData : 0)",
}

# void** out-parameters that hand back an interface without an IID.
VOID_OUT_OBJECT = {"GetDDInterface"}

NORETURN = {"ExitProcess"}

# Functions that create a handle: the handle gets a fresh id on every
# creation, so ids follow handle lifetimes rather than raw values (Wine reuses
# raw HMMIO/HKEY values after close, at addresses that depend on unrelated
# allocations). "return" or the out-parameter's name.
HANDLE_NEW = {"mmioOpenA": "return", "CreateWindowExA": "return", "CreateRectRgn": "return",
              "AVIStreamGetFrameOpen": "return", "GetDC": "return", "RegCreateKeyA": "phkResult",
              "mixerOpen": "phmx", "AVIStreamOpenFromFileA": "ppavi"}

# Flat functions whose out-parameters are only meaningful on this condition
# (default: SUCCEEDED for HRESULT, always otherwise).
SUCCESS = {name: "sr_r == 0" for name in (
    "RegCreateKeyA", "RegQueryValueExA", "mmioGetInfo", "mmioDescend", "mmioAscend", "mixerOpen",
    "mixerGetDevCapsA", "mixerGetLineInfoA", "mixerGetLineControlsA", "mixerGetControlDetailsA", "mciSendStringA")}
SUCCESS.update({"mciGetErrorStringA": "sr_r", "GetCursorPos": "sr_r", "GetClientRect": "sr_r",
                "GetWindowRect": "sr_r"})

LP_ALIAS = {
    "LPDWORD": "DWORD *", "LPLONG": "LONG *", "LPVOID": "void *", "LPCVOID": "const void *",
    "LPSTR": "char *", "LPCSTR": "const char *", "HPSTR": "char *", "LPWSTR": "WCHAR *",
    "LPCWSTR": "const WCHAR *", "LPUNKNOWN": "IUnknown *", "LPGUID": "GUID *", "LPCGUID": "const GUID *",
    "LPBYTE": "BYTE *", "PHKEY": "HKEY *", "LPD3DVALUE": "D3DVALUE *", "REFIID": "const IID *",
    "REFCLSID": "const IID *", "REFGUID": "const GUID *", "REFMSPID": "const MSPID *",
    "LPHMIXER": "HMIXER *", "LPLPDIRECTSOUNDBUFFER": "IDirectSoundBuffer **",
    "LPCMMIOINFO": "const MMIOINFO *", "LPPOINT": "POINT *", "LPRECT": "RECT *", "LPMSG": "MSG *",
    "LPWORD": "WORD *", "LPBOOL": "WINBOOL *",
}
GUIDS = {"IID", "GUID", "CLSID", "MSPID"}
HANDLES = {"HWND", "HDC", "HANDLE", "HMODULE", "HINSTANCE", "HRSRC", "HMMIO", "HMIXER", "HMIXEROBJ", "HKEY",
           "HRGN", "HCURSOR", "HMENU", "HICON", "PAVISTREAM", "PGETFRAME", "HGLOBAL", "HBITMAP", "HPALETTE"}
F32 = {"D3DVALUE", "float", "FLOAT"}
F64 = {"double"}
I64 = {"STREAM_TIME", "LONGLONG"}
HEX = {"D3DCOLOR", "WPARAM", "LPARAM"}
U32 = {"DWORD", "ULONG", "UINT", "WORD", "BYTE", "WINBOOL", "BOOL", "unsigned", "ULONG_PTR", "DWORD_PTR",
       "UINT_PTR", "MMRESULT", "ATOM", "SIZE_T", "D3DTEXTUREHANDLE", "D3DMATERIALHANDLE", "D3DMATRIXHANDLE",
       "MCIERROR", "UCHAR", "USHORT", "D3DRMGROUPINDEX"}
I32 = {"int", "LONG", "INT", "INT_PTR", "LRESULT", "HRESULT", "SHORT", "LONG_PTR"}
STRUCT_FORMATTERS = {"RECT", "POINT", "D3DVECTOR", "D3DRMBOX", "D3DRMQUATERNION", "D3DMATRIX", "DDCOLORKEY",
                     "DDPIXELFORMAT", "DDSURFACEDESC", "DDSURFACEDESC2", "DDSCAPS", "DDSCAPS2", "DDBLTFX",
                     "WAVEFORMATEX", "DSBUFFERDESC", "MSG", "WNDCLASSA", "D3DRMIMAGE", "MMCKINFO", "MMIOINFO",
                     "D3DVIEWPORT2"}


class Ctx:
    def __init__(self):
        self.ifaces = {}       # name -> (has_iid, [(ret, meth, [(type, name)])])
        self.order = []
        self.lptypes = {}
        self.callbacks = {}    # name -> (conv, ret, [(type, name)])
        self.flat = []         # (section kind, dll, ret, name, params, hook)


def parse_params(s):
    s = s.strip()
    if not s:
        return []
    out = []
    for p in s.split(","):
        p = p.strip()
        m = re.match(r"^(.*?)\s*([A-Za-z_]\w*)$", p)
        out.append((m.group(1).strip(), m.group(2)))
    return out


def load(ctx):
    cur = None
    for line in (HERE / "methods.txt").read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        if line.startswith("interface "):
            parts = line.split()
            cur = parts[1]
            ctx.ifaces[cur] = ("iid" in parts[2:], [])
            ctx.order.append(cur)
        elif line.startswith("  "):
            m = re.match(r"^\s+(.*?)\s*\b(\w+)\((.*)\)$", line)
            ctx.ifaces[cur][1].append((m.group(1), m.group(2), parse_params(m.group(3))))
        elif line.startswith("lptype "):
            _, lp, iface = line.split()
            ctx.lptypes[lp] = iface
        elif line.startswith("callback "):
            parse_callback(ctx, line)
    section = None
    for line in (HERE / "flat.txt").read_text().splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if line.startswith(("proxy ", "iat ")):
            section = line.split()
        elif line.startswith("callback "):
            parse_callback(ctx, line)
        else:
            m = re.match(r"^\s+(.*?)\s*\b(\w+)\((.*)\)(\s+hook)?$", line)
            ctx.flat.append((section[0], section[1], m.group(1), m.group(2), parse_params(m.group(3)),
                             bool(m.group(4))))


def parse_callback(ctx, line):
    m = re.match(r"^callback (\w+) (\w+) (.*?)\((.*)\)$", line)
    ctx.callbacks[m.group(1)] = (m.group(2), m.group(3).strip(), parse_params(m.group(4)))


def expand(ctx, t):
    """-> (base, depth, const)"""
    t = re.sub(r"\bstruct\s+", "", t).strip()
    const = bool(re.match(r"^const\b", t))
    t = re.sub(r"\bconst\b", "", t)
    depth = t.count("*")
    base = t.replace("*", "").strip()
    for _ in range(3):
        if base in LP_ALIAS:
            b2, d2, c2 = expand(ctx, LP_ALIAS[base])
            base, depth, const = b2, depth + d2, const or c2
        elif base in ctx.lptypes:
            base, depth = ctx.lptypes[base], depth + 1
        elif (re.match(r"^LPC?[A-Z0-9_]+$", base) and base not in ctx.callbacks
              and base not in HEX | U32 | I32 | HANDLES
              and not base.endswith(("CALLBACK", "CALLBACKA", "CALLBACK2"))):
            name = base[2:]
            if name.startswith("C") and name[1:] in STRUCT_FORMATTERS:
                name, const = name[1:], True
            base, depth = name, depth + 1
        else:
            break
    return base, depth, const


def is_iface_like(base):
    return re.match(r"^I[A-Z][A-Za-z0-9]+$", base) is not None


def classify(ctx, method, params, i):
    t, name = params[i]
    if (method, name) in PARAM_KIND:
        return PARAM_KIND[(method, name)], SPARSE.get((method, name))
    base, depth, const = expand(ctx, t)
    count = COUNTS.get((method, name))
    if count and not all(w in {n for _, n in params} for w in re.findall(r"[A-Za-z_]\w*", count)):
        count = None
    if depth == 0 and (base in ctx.callbacks):
        return "cb", base
    if depth == 0 and (base.endswith("PROC") or "CALLBACK" in base):
        return "cb_unsupported", base
    if base == "D3DRMMATRIX4D" and depth == 0:
        return "mat4", None
    if base in GUIDS:
        return ("guid", None) if depth == 1 else ("ptr", None)
    if base in ctx.ifaces:
        if depth == 1:
            return "obj_in", base
        if depth == 2:
            return ("objarr_out", (base, count)) if count else ("obj_out", base)
    if is_iface_like(base) and base not in ctx.ifaces:
        return {1: "xobj_in", 2: "xobj_out"}.get(depth, "ptr"), base
    if base == "void":
        if depth == 1:
            return "ptr", None
        if depth == 2:
            for j in range(i - 1, -1, -1):
                b2, d2, _ = expand(ctx, params[j][0])
                if b2 in GUIDS and d2 == 1:
                    return "qi_out", params[j][1]
            return ("void_obj_out" if method in VOID_OUT_OBJECT else "void_out"), None
    if base in GUIDS:
        return ("guid", None) if depth == 1 else ("ptr", None)
    if base in ("char", "CHAR") and depth == 1:
        return ("str", None) if const else ("strout", None)
    if base == "WCHAR" and depth == 1:
        return "wstr", None
    if base in HANDLES:
        return {0: "handle", 1: "handle_out"}.get(depth, "ptr"), None
    for kinds, k in ((F32, "f32"), (F64, "f64"), (I64, "i64"), (HEX, "hex"), (U32, "u32"), (I32, "i32")):
        if base in kinds:
            if depth == 0:
                return k, None
            if depth == 1:
                if base == "BYTE":
                    return ("bytes", count) if count else ("ptr", None)
                return "p" + k, None
            return "ptr", None
    if depth == 0:
        return "enum", None
    if depth == 1:
        return "pstruct", base
    return "ptr", None


def ret_kind(ctx, method, ret):
    if method in RET_KIND:
        return RET_KIND[method]
    if ret == "void":
        return "void"
    k, info = classify(ctx, method, [(ret, "r")], 0)
    return {"strout": "str", "pstruct": "pstruct_ret"}.get(k, k)


FMT_SCALAR = {
    "f32": "sr_f_f32(&sr_c.b, {v});", "f64": "sr_f_f64(&sr_c.b, {v});", "i64": "sr_f_i64(&sr_c.b, (long long)({v}));",
    "hex": "sr_f_hex(&sr_c.b, (unsigned)({v}));", "u32": "sr_f_u32(&sr_c.b, (unsigned)({v}));",
    "i32": "sr_f_i32(&sr_c.b, (int)({v}));",
    "enum": "_Static_assert(__builtin_classify_type({v}) <= 3, \"non-integral by-value type\"); "
            "sr_f_i32(&sr_c.b, (int)({v}));",
}


def fmt_value(ctx, kind, info, v, name, ok="sr_ok"):
    """C statements appending the printed value of expression v (post-call)."""
    if kind in FMT_SCALAR:
        return FMT_SCALAR[kind].format(v=v)
    if kind.startswith("p") and kind[1:] in FMT_SCALAR and kind not in ("pstruct",):
        inner = FMT_SCALAR[kind[1:]].format(v=f"*{v}")
        return f'if ({v}) {{ sr_put(&sr_c.b, "&"); {inner} }} else sr_put(&sr_c.b, "NULL");'
    if kind in ("obj_in", "xobj_in"):
        return f"sr_f_obj(&sr_c.b, {v});"
    if kind in ("obj_out", "xobj_out", "qi_out", "void_obj_out"):
        return f'if ({v}) {{ sr_put(&sr_c.b, "&"); sr_f_obj(&sr_c.b, *{v}); }} else sr_put(&sr_c.b, "NULL");'
    if kind == "objarr_out":
        cnt = info[1]
        return (f'if ({v}) {{ unsigned n_ = {ok} ? (unsigned)({cnt}) : 0; sr_put(&sr_c.b, "["); '
                f'for (unsigned i_ = 0; i_ < n_; i_++) {{ if (i_) sr_put(&sr_c.b, " "); sr_f_obj(&sr_c.b, {v}[i_]); }} '
                f'sr_put(&sr_c.b, "]"); }} else sr_put(&sr_c.b, "NULL");')
    if kind == "void_out":
        return f'if ({v}) {{ sr_put(&sr_c.b, "&"); sr_f_ptr(&sr_c.b, *{v}); }} else sr_put(&sr_c.b, "NULL");'
    if kind in ("ptr", "cb", "cb_unsupported"):
        return f"sr_f_ptr(&sr_c.b, (const void*)({v}));"
    if kind == "ptrval":
        return f"sr_f_ptr(&sr_c.b, (const void*)(uintptr_t)({v}));"
    if kind == "guid":
        return f"sr_f_guid(&sr_c.b, (const GUID*)({v}));"
    if kind == "str":
        return f"sr_f_str(&sr_c.b, {v});"
    if kind == "strout":
        return f'if ({ok}) sr_f_str(&sr_c.b, {v}); else sr_f_ptr(&sr_c.b, {v});'
    if kind == "wstr":
        return f"sr_f_wstr(&sr_c.b, {v});"
    if kind == "handle":
        return f"sr_f_handle(&sr_c.b, (const void*)(uintptr_t)({v}));"
    if kind == "handle_out":
        return (f'if ({v}) {{ sr_put(&sr_c.b, "&"); sr_f_handle(&sr_c.b, (const void*)(uintptr_t)*{v}); }} '
                f'else sr_put(&sr_c.b, "NULL");')
    if kind == "mat4":
        return f"sr_f_floats(&sr_c.b, (const float*)({v}), 16);"
    if kind == "pstruct":
        return f'sr_f_struct(&sr_c.b, "{info}", {v}, sizeof(*{v}));'
    if kind == "pstruct_ret":
        return f'sr_f_struct(&sr_c.b, "{info}", {v}, {v} ? sizeof(*{v}) : 0);'
    if kind == "sparse":
        return (f'if ({v} && {ok}) {{ unsigned n_ = (unsigned)({info}); const unsigned char* p_ = (const unsigned char*){v}; '
                f'sr_putf(&sr_c.b, "[%u]{{", n_); int f_ = 1; for (unsigned i_ = 0; i_ < n_; i_++) if (p_[i_]) '
                f'{{ sr_putf(&sr_c.b, f_ ? "%x:%x" : " %x:%x", i_, p_[i_]); f_ = 0; }} sr_put(&sr_c.b, "}}"); }} '
                f'else sr_f_ptr(&sr_c.b, {v});')
    if kind == "bytes":
        return (f'if ({v} && {ok}) {{ unsigned n_ = (unsigned)({info}); sr_putf(&sr_c.b, "[%u]", n_); '
                f'sr_f_bytes(&sr_c.b, {v}, n_ > 64 ? 64 : n_); }} else sr_f_ptr(&sr_c.b, {v});')
    raise SystemExit(f"no formatter for kind {kind}")


def find_ctx_param(ctx, params, i):
    """The void* context argument that travels with callback parameter i."""
    for j in (i + 1, i - 1):
        if 0 <= j < len(params):
            b, d, _ = expand(ctx, params[j][0])
            if b == "void" and d == 1:
                return j
    return None


def cb_ctx_index(ctx, cbname):
    conv, ret, params = ctx.callbacks[cbname]
    idx = [i for i, (t, n) in enumerate(params) if expand(ctx, t)[:2] == ("void", 1)]
    return idx[0] if len(idx) == 1 else None


def emit_body(ctx, out, label_iface, label_name, ret, params, real_call, hook_call, this_fmt, method,
              release=False, noreturn=False, addref=False):
    """Common wrapper body. params excludes `this`. real_call: format string taking the arg list."""
    rk = ret_kind(ctx, method, ret)
    kinds = [classify(ctx, method, params, i) for i in range(len(params))]
    out.append(f'    SRCall sr_c; sr_begin(&sr_c, SR_RA(), {label_iface}, "{label_name}");')
    if this_fmt:
        out.append("    unsigned self_id_ = sr_w->id;")
    args = []
    pre = []
    for i, ((t, n), (k, info)) in enumerate(zip(params, kinds)):
        if k in ("obj_in", "xobj_in"):
            args.append(f"(__typeof__({n}))sr_unwrap((void*){n})")
        elif k == "cb":
            j = find_ctx_param(ctx, params, i)
            if j is None or cb_ctx_index(ctx, info) is None:
                pre.append(f'    if ({n} && sr_c.game) sr_log_warn("UNSUPPORTED callback {info} in {label_name}: '
                           f'game callbacks through it are not wrapped");')
                args.append(n)
            else:
                cn = params[j][1]
                pre.append(f"    void* cbctx_{n} = {n} ? (void*)sr_closure((void*){n}, (void*){cn}, "
                           f"SRCB_KIND_{info}) : (void*){cn};")
                args.append(f"{n} ? (__typeof__({n}))SRCB_{info} : NULL")
        elif k == "cb_unsupported":
            pre.append(f'    if ({n} && sr_c.game) sr_log_warn("UNSUPPORTED callback {info} in {label_name}: '
                       f'game callbacks through it are not wrapped");')
            args.append(n)
        else:
            args.append(n)
    # context parameters of wrapped callbacks are replaced by their closure
    for i, ((t, n), (k, info)) in enumerate(zip(params, kinds)):
        if k == "cb":
            j = find_ctx_param(ctx, params, i)
            if j is not None and cb_ctx_index(ctx, info) is not None:
                args[j] = f"(__typeof__({params[j][1]}))cbctx_{n}"
    out.extend(pre)
    call = (hook_call if hook_call else real_call).format(args=", ".join(args))

    def fmt_line(ok):
        lines = ["    if (sr_c.log) {"]
        if this_fmt:
            lines.append('        sr_putf(&sr_c.b, "#%u", self_id_);')
        for i, ((t, n), (k, info)) in enumerate(zip(params, kinds)):
            if this_fmt or i:
                lines.append("        sr_arg_sep(&sr_c);")
            lines.append("        " + fmt_value(ctx, k, info, n, n, ok))
        return lines

    if noreturn:
        out.extend(fmt_line("1"))
        out.append('        sr_put(&sr_c.b, ") = noreturn");')
        out.append("    }")
        out.append("    sr_end(&sr_c);")
        out.append(f"    {call};")
        return
    if rk == "void":
        out.append(f"    {call};")
        out.append("    int sr_ok = 1;")
    else:
        out.append(f"    {ret} sr_r = {call};")
        out.append("    int sr_ok = " + (f"({SUCCESS[method]});" if method in SUCCESS else
                                     "SUCCEEDED(sr_r);" if ret == "HRESULT" else "1;"))
    out.append("    (void)sr_ok;")
    # wrap out objects for the game
    for (t, n), (k, info) in zip(params, kinds):
        if k == "obj_out":
            idx = ctx.order.index(info)
            out.append(f"    if (sr_c.game && sr_ok && {n} && *{n}) *{n} = (__typeof__(*{n}))sr_wrap_out((void*)*{n}, {idx});")
        elif k == "objarr_out":
            idx = ctx.order.index(info[0])
            out.append(f"    if (sr_c.game && sr_ok && {n}) {{ unsigned n_ = (unsigned)({info[1]}); for (unsigned i_ = 0; i_ < n_; i_++) "
                       f"{n}[i_] = (__typeof__(*{n}))sr_wrap_out((void*){n}[i_], {idx}); }}")
        elif k == "qi_out":
            out.append(f"    if (sr_c.game && sr_ok && {n} && *{n}) *{n} = sr_wrap_iid_out(*{n}, (const IID*){info});")
        elif k == "void_obj_out":
            out.append(f"    if (sr_c.game && sr_ok && {n} && *{n}) *{n} = sr_wrap_identify(*{n});")
        elif k == "xobj_out":
            out.append(f'    if (sr_c.game && sr_ok && {n} && *{n}) sr_log_warn("UNWRAPPED {info} returned by {label_name}: '
                       f'calls on it are not logged");')
    if HANDLE_NEW.get(method) == "return":
        out.append("    if (sr_c.game && sr_r) sr_handle_new((const void*)(uintptr_t)sr_r);")
    elif method in HANDLE_NEW:
        n = HANDLE_NEW[method]
        out.append(f"    if (sr_c.game && sr_ok && {n} && *{n}) sr_handle_new((const void*)(uintptr_t)*{n});")
    out.extend(fmt_line("sr_ok"))
    if rk != "void":
        out.append('        sr_put(&sr_c.b, ") = ");')
        if rk == "hr" or ret == "HRESULT":
            out.append("        sr_f_hr(&sr_c.b, sr_r);")
        else:
            out.append("        " + fmt_value(ctx, rk, kind_info(ctx, method, ret), "sr_r", "sr_r"))
    else:
        out.append('        sr_put(&sr_c.b, ")");')
    out.append("    }")
    if release:
        out.append("    if (sr_c.game) sr_on_release(sr_w, sr_r);")
    if addref:
        out.append("    if (sr_c.game) sr_w->grefs++;")
    out.append("    sr_end(&sr_c);")
    if rk != "void":
        out.append("    return sr_r;")


def kind_info(ctx, method, ret):
    return classify(ctx, method, [(ret, "r")], 0)[1]


def iface_sig(ctx, name):
    return [(m, tuple(t for t, _ in p), r) for r, m, p in ctx.ifaces[name][1]]


def gen_wrappers(ctx):
    out = ['/* Generated by tools/seamrec/gen.py from methods.txt. Do not edit. */', '#include "sr.h"', ""]
    # callback kinds + trampolines
    cbs = sorted(ctx.callbacks)
    for cb in cbs:
        conv, ret, params = ctx.callbacks[cb]
        ci = cb_ctx_index(ctx, cb)
        if ci is None:
            continue
        cc = "__stdcall" if conv == "stdcall" else "__cdecl"
        plist = ", ".join(f"{t} {n}" for t, n in params)
        out.append(f"{ret} {cc} SRCB_{cb}({plist})")
        out.append("{")
        ctxn = params[ci][1]
        out.append(f"    SRClosure* sr_cl = (SRClosure*){ctxn};")
        out.append(f'    SRCall sr_c; sr_begin_cb(&sr_c, "{cb}");')
        kinds = [classify(ctx, cb, params, i) for i in range(len(params))]
        args = []
        for i, ((t, n), (k, info)) in enumerate(zip(params, kinds)):
            if i == ci:
                args.append(f"sr_cl->ctx")
            elif k == "obj_in":
                if cb == "D3DRMLOADCALLBACK":
                    out.append(f"    {n} = (__typeof__({n}))sr_wrap_iid((void*){n}, objectguid);")
                else:
                    out.append(f"    {n} = (__typeof__({n}))sr_wrap((void*){n}, {ctx.order.index(info)});")
                args.append(n)
            else:
                args.append(n)
        call = f"(({cb})sr_cl->fn)({', '.join(args)})"
        if ret == "void":
            out.append(f"    {call};")
        else:
            out.append(f"    {ret} sr_r = {call};")
        out.append("    int sr_ok = 1; (void)sr_ok;")
        out.append("    if (sr_c.log) {")
        out.append("        sr_f_fn(&sr_c.b, sr_cl->fn);")
        for i, ((t, n), (k, info)) in enumerate(zip(params, kinds)):
            out.append("        sr_arg_sep(&sr_c);")
            if i == ci:
                out.append("        sr_f_ptr(&sr_c.b, sr_cl->ctx);")
            else:
                out.append("        " + fmt_value(ctx, k, info, n, n))
        if ret == "void":
            out.append('        sr_put(&sr_c.b, ")");')
        else:
            out.append('        sr_put(&sr_c.b, ") = ");')
            rk = ret_kind(ctx, cb, ret)
            out.append("        " + ("sr_f_hr(&sr_c.b, sr_r);" if ret == "HRESULT" else fmt_value(ctx, rk, None, "sr_r", "sr_r")))
        out.append("    }")
        for (t, n), (k, info) in zip(params, kinds):
            if k == "obj_out":
                out.append(f"    if ({n} && *{n}) *{n} = (__typeof__(*{n}))sr_unwrap((void*)*{n});")
        out.append("    sr_end(&sr_c);")
        if ret != "void":
            out.append("    return sr_r;")
        out.append("}")
        out.append("")
    # methods
    for iface in ctx.order:
        has_iid, methods = ctx.ifaces[iface]
        for ret, meth, params in methods:
            plist = "".join(f", {t} {n}" for t, n in params)
            out.append(f"static {ret} __stdcall W_{iface}_{meth}(SRWrap* sr_w{plist})")
            out.append("{")
            real = f"(({iface}*)sr_w->real)->lpVtbl->{meth}(({iface}*)sr_w->real{{sep}}{{{{args}}}})"
            real = real.replace("{sep}", ", " if params else "")
            real = real.replace("{{args}}", "{args}")
            hook = None
            if meth in HOOKS.get(iface, ()):
                hook = f"SRH_{iface}_{meth}(sr_w{', ' if params else ''}{{args}})"
            emit_body(ctx, out, f'"{iface}"', meth, ret, params, real, hook, True, meth,
                      release=(meth == "Release"), addref=(meth == "AddRef"))
            out.append("}")
            out.append("")
        out.append(f"static const void* const vt_{iface}[] = {{")
        for ret, meth, params in methods:
            out.append(f"    (const void*)W_{iface}_{meth},")
        out.append("};")
        out.append("")
    # ancestors
    sigs = {n: iface_sig(ctx, n) for n in ctx.order}
    for iface in ctx.order:
        row = []
        for other in ctx.order:
            a, b = sigs[other], sigs[iface]
            row.append("1" if other != iface and len(a) <= len(b) and b[:len(a)] == a else "0")
        out.append(f"static const unsigned char anc_{iface}[] = {{{', '.join(row)}}};")
    out.append("")
    out.append("const SRIface sr_ifaces[] = {")
    for iface in ctx.order:
        has_iid = ctx.ifaces[iface][0]
        iid = f"&IID_{iface}" if has_iid else "NULL"
        out.append(f'    {{ "{iface}", {iid}, vt_{iface}, {len(ctx.ifaces[iface][1])}, anc_{iface} }},')
    out.append("};")
    out.append(f"const int sr_niface = {len(ctx.order)};")
    return "\n".join(out) + "\n"


def gen_flat(ctx):
    out = ['/* Generated by tools/seamrec/gen.py from flat.txt. Do not edit. */', '#include "sr.h"', ""]
    iat = []
    for kind, dll, ret, name, params, hook in ctx.flat:
        plist = ", ".join(f"{t} {n}" for t, n in params) or "void"
        args = ", ".join(n for _, n in params)
        if kind == "proxy":
            fn = f"SR_{dll}_{name}"
            out.append(f"SR_EXPORT {ret} WINAPI {fn}({plist})")
            out.append("{")
            out.append(f'    if (!sr_real_{name}) sr_real_{name} = (__typeof__(sr_real_{name}))sr_real_proc("{dll}", "{name}");')
            out.append("    if (!sr_is_game(SR_RA())) " + ("{ " if ret == "void" else "return ") +
                       f"sr_real_{name}({args});" + (" return; }" if ret == "void" else ""))
        else:
            fn = f"SRI_{name}"
            out.append(f"static {ret} WINAPI {fn}({plist})")
            out.append("{")
            iat.append((dll, name))
        real = f"sr_real_{name}({{args}})"
        hookc = f"SRH_{name}({{args}})" if hook else None
        emit_body(ctx, out, "NULL", name, ret, params, real, hookc, False, name, noreturn=name in NORETURN)
        out.append("}")
        if name not in NORETURN:
            out.append(f"static __typeof__(&{name}) const sr_chk_{name} __attribute__((unused)) = {fn};")
        out.append("")
    out.append("static const struct { const char* dll; const char* name; void** real; void* hook; } sr_iat[] = {")
    for dll, name in iat:
        out.append(f'    {{ "{dll}", "{name}", (void**)&sr_real_{name}, (void*)SRI_{name} }},')
    out.append("};")
    out.append(f"const int sr_niat = {len(iat)};")
    out.append("""
void sr_install_iat(HMODULE exe)
{
    char* base = (char*)exe;
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)exe)->e_lfanew);
    IMAGE_DATA_DIRECTORY* dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    int patched = 0;
    for (IMAGE_IMPORT_DESCRIPTOR* d = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir->VirtualAddress); d->Name; d++) {
        const char* dll = base + d->Name;
        IMAGE_THUNK_DATA* names = (IMAGE_THUNK_DATA*)(base + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        IMAGE_THUNK_DATA* slots = (IMAGE_THUNK_DATA*)(base + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, slots++) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            const char* fn = (const char*)((IMAGE_IMPORT_BY_NAME*)(base + names->u1.AddressOfData))->Name;
            for (int i = 0; i < sr_niat; i++) {
                if (_stricmp(sr_iat[i].dll, dll) || strcmp(sr_iat[i].name, fn)) continue;
                DWORD old;
                VirtualProtect(&slots->u1.Function, sizeof(void*), PAGE_READWRITE, &old);
                *sr_iat[i].real = (void*)slots->u1.Function;
                slots->u1.Function = (uintptr_t)sr_iat[i].hook;
                VirtualProtect(&slots->u1.Function, sizeof(void*), old, &old);
                patched++;
            }
        }
    }
    if (patched != sr_niat) sr_side_warn("IAT: patched %d of %d hooks", patched, sr_niat);
}
""")
    return "\n".join(out) + "\n"


def gen_hooks_h(ctx):
    out = ["/* Generated by tools/seamrec/gen.py. Do not edit. */", "#ifndef SEAMREC_GEN_HOOKS_H",
           "#define SEAMREC_GEN_HOOKS_H", "int sr_is_game(void* ra);", "void* sr_wrap_identify(void* real);"]
    for kind, dll, ret, name, params, hook in ctx.flat:
        out.append(f"extern __typeof__(&{name}) sr_real_{name};")
        if hook:
            plist = ", ".join(f"{t} {n}" for t, n in params) or "void"
            out.append(f"{ret} WINAPI SRH_{name}({plist});")
    for iface, meths in sorted(HOOKS.items()):
        for ret, meth, params in ctx.ifaces[iface][1]:
            if meth in meths:
                plist = "".join(f", {t} {n}" for t, n in params)
                out.append(f"{ret} SRH_{iface}_{meth}(SRWrap* w{plist});")
    for i, cb in enumerate(sorted(ctx.callbacks)):
        conv, ret, params = ctx.callbacks[cb]
        out.append(f"#define SRCB_KIND_{cb} {i}")
        if cb_ctx_index(ctx, cb) is not None:
            cc = "__stdcall" if conv == "stdcall" else "__cdecl"
            out.append(f"{ret} {cc} SRCB_{cb}({', '.join(f'{t} {n}' for t, n in params)});")
    out.append("#endif")
    defs = ["/* Generated by tools/seamrec/gen.py. Do not edit. */", '#include "sr.h"']
    for kind, dll, ret, name, params, hook in ctx.flat:
        defs.append(f"__typeof__(&{name}) sr_real_{name};")
    return "\n".join(out) + "\n", "\n".join(defs) + "\n"


def gen_proxy(ctx, dll):
    exports = []
    for line in (HERE / "exports" / f"{dll}.txt").read_text().split("\n"):
        if line.strip():
            o, n = line.split()
            exports.append((int(o), None if n == "-" else n))
    wrapped = {name for kind, d, ret, name, params, hook in ctx.flat if kind == "proxy" and d == dll}
    missing = wrapped - {n for _, n in exports}
    if missing:
        raise SystemExit(f"{dll}: wrapped functions missing from exports/{dll}.txt: {missing}")
    c = [f"/* Generated by tools/seamrec/gen.py: pass-through proxy for {dll}.dll. Do not edit. */",
         "#include <windows.h>",
         "__declspec(dllimport) HMODULE SR_LoadReal(const char* dll);",
         "__declspec(dllimport) void SR_MissingExport(void);"]
    passthrough = [(o, n) for o, n in exports if n not in wrapped]
    c.append(f"static FARPROC slots[{max(len(passthrough), 1)}];")
    for i, (o, n) in enumerate(passthrough):
        c.append(f'__attribute__((naked)) void P_{i}(void) {{ __asm__ volatile("jmp *%0" : : "m"(slots[{i}])); }}')
    c.append("static const struct { const char* name; WORD ord; } names[] = {")
    for o, n in passthrough:
        c.append(f'    {{ {chr(34) + n + chr(34) if n else "NULL"}, {o} }},')
    c.append("    { NULL, 0 } };")
    c.append(f"""
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {{
        DisableThreadLibraryCalls(inst);
        HMODULE real = SR_LoadReal("{dll}");
        for (int i = 0; i < {len(passthrough)}; i++) {{
            slots[i] = GetProcAddress(real, names[i].name ? names[i].name : (LPCSTR)(ULONG_PTR)names[i].ord);
            if (!slots[i]) slots[i] = (FARPROC)SR_MissingExport;
        }}
    }}
    return TRUE;
}}
""")
    d = [f"LIBRARY {dll}.dll", "EXPORTS"]
    pi = 0
    for o, n in exports:
        if n in wrapped:
            d.append(f"  {n} = seamrec.SR_{dll}_{n} @{o}")
        else:
            label = n or f"__noname_{o}"
            d.append(f"  {label} = P_{pi} @{o}" + ("" if n else " NONAME"))
            pi += 1
    return "\n".join(c) + "\n", "\n".join(d) + "\n"


def main(argv):
    outdir = pathlib.Path(argv[1])
    outdir.mkdir(parents=True, exist_ok=True)
    ctx = Ctx()
    load(ctx)
    (outdir / "gen_wrappers.c").write_text(gen_wrappers(ctx))
    (outdir / "gen_flat.c").write_text(gen_flat(ctx))
    h, defs = gen_hooks_h(ctx)
    (outdir / "gen_hooks.h").write_text(h)
    (outdir / "gen_reals.c").write_text(defs)
    dlls = sorted({d for k, d, *_ in ctx.flat if k == "proxy"})
    for dll in dlls:
        c, d = gen_proxy(ctx, dll)
        (outdir / f"proxy_{dll}.c").write_text(c)
        (outdir / f"{dll}.def").write_text(d)
    (outdir / "proxies.txt").write_text("\n".join(dlls) + "\n")


if __name__ == "__main__":
    main(sys.argv)
