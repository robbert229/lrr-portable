#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Extract the method table (methods.txt) from the preprocessed DX headers.

    i686-w64-mingw32-gcc -E -P -x c src/dxall.h | python3 extract.py > methods.txt

Every interface in INTERFACES is located as a C vtable struct (`struct
IFooVtbl { RET (__stdcall *Method)(IFoo *This, ...); ... }`), which is the
uniform shape both DECLARE_INTERFACE_ and MIDL headers expand to under
CINTERFACE. The output lists each interface's full vtable in slot order, with
every parameter named (unnamed ones become a1, a2, ...). methods.txt is
committed: it is the reviewable source gen.py emits the wrappers from, and it
pins the signatures independently of later header changes.

Also emitted: `lptype LPFOO IFoo` lines mapping pointer typedefs to their
interface, and `callback NAME RET(PARAMS)` lines for every function-pointer
typedef that a wrapped method takes.
"""
import re
import sys

# The interfaces the recorder wraps. Order is the order in methods.txt.
INTERFACES = [
    "IUnknown",
    # DirectDraw (ddraw.h)
    "IDirectDraw", "IDirectDraw2", "IDirectDraw4",
    "IDirectDrawSurface", "IDirectDrawSurface2", "IDirectDrawSurface3", "IDirectDrawSurface4",
    "IDirectDrawPalette", "IDirectDrawClipper", "IDirectDrawGammaControl",
    # Direct3D immediate mode (d3d.h)
    "IDirect3D", "IDirect3D2", "IDirect3D3",
    "IDirect3DDevice", "IDirect3DDevice2", "IDirect3DDevice3",
    "IDirect3DViewport", "IDirect3DViewport2", "IDirect3DViewport3",
    "IDirect3DTexture", "IDirect3DTexture2",
    "IDirect3DMaterial", "IDirect3DMaterial2", "IDirect3DMaterial3",
    "IDirect3DLight", "IDirect3DExecuteBuffer", "IDirect3DVertexBuffer",
    # Direct3D retained mode (d3drm.h, d3drmobj.h)
    "IDirect3DRM", "IDirect3DRM2", "IDirect3DRM3",
    "IDirect3DRMObject", "IDirect3DRMObject2", "IDirect3DRMVisual",
    "IDirect3DRMDevice", "IDirect3DRMDevice2", "IDirect3DRMDevice3",
    "IDirect3DRMViewport", "IDirect3DRMViewport2",
    "IDirect3DRMFrame", "IDirect3DRMFrame2", "IDirect3DRMFrame3",
    "IDirect3DRMMesh", "IDirect3DRMShadow", "IDirect3DRMShadow2",
    "IDirect3DRMFace", "IDirect3DRMFace2",
    "IDirect3DRMMeshBuilder", "IDirect3DRMMeshBuilder2", "IDirect3DRMMeshBuilder3",
    "IDirect3DRMLight", "IDirect3DRMTexture", "IDirect3DRMTexture2", "IDirect3DRMTexture3",
    "IDirect3DRMWrap", "IDirect3DRMMaterial", "IDirect3DRMMaterial2",
    "IDirect3DRMAnimation", "IDirect3DRMAnimation2",
    "IDirect3DRMAnimationSet", "IDirect3DRMAnimationSet2",
    "IDirect3DRMUserVisual", "IDirect3DRMArray",
    "IDirect3DRMDeviceArray", "IDirect3DRMFrameArray", "IDirect3DRMViewportArray",
    "IDirect3DRMVisualArray", "IDirect3DRMAnimationArray", "IDirect3DRMPickedArray",
    "IDirect3DRMPicked2Array", "IDirect3DRMLightArray", "IDirect3DRMFaceArray",
    "IDirect3DRMProgressiveMesh", "IDirect3DRMClippedVisual",
    # DirectInput 5 (dinput.h)
    "IDirectInputA", "IDirectInput2A", "IDirectInputDeviceA", "IDirectInputDevice2A",
    # DirectSound (dsound.h)
    "IDirectSound", "IDirectSoundBuffer", "IDirectSound3DListener", "IDirectSound3DBuffer",
    "IDirectSoundNotify",
    # DirectShow multimedia streaming, used by the movie player (mmstream.h, amstream.h, ddstream.h)
    "IMultiMediaStream", "IAMMultiMediaStream", "IMediaStream", "IStreamSample",
    "IDirectDrawMediaStream", "IDirectDrawStreamSample", "IAMMediaStream",
]

KEYWORDS = {"const", "struct", "unsigned", "signed", "volatile", "enum"}


def norm_type(t):
    t = re.sub(r"__attribute__\(\(\w+\)\)", "", t)
    t = re.sub(r"\s*\*\s*", " *", t)
    t = re.sub(r"\s+", " ", t).strip()
    t = t.replace("* *", "**").replace("* *", "**")
    t = re.sub(r"\*\s+const\b", "*const", t)
    return t


def split_top(s, sep=","):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == sep and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return out


def parse_param(p, idx):
    p = p.strip()
    arr = ""
    m = re.search(r"((\[\w*\])+)$", p)
    if m:
        arr, p = m.group(1), p[:m.start()].strip()
    m = re.match(r"^(.*?)([A-Za-z_]\w*)$", p)
    if m:
        head = m.group(1).strip()
        words = re.findall(r"[A-Za-z_]\w*", head)
        if [w for w in words if w not in KEYWORDS] or "unsigned" in words or "signed" in words:
            return norm_type(head + arr), m.group(2)
    return norm_type(p + arr), f"a{idx}"


def parse_vtables(text):
    found = {}
    for m in re.finditer(r"struct\s+(\w+)Vtbl\s*\{", text):
        name = m.group(1)
        start, depth, i = m.end(), 1, m.end()
        while depth:
            depth += {"{": 1, "}": -1}.get(text[i], 0)
            i += 1
        body = text[start:i - 1]
        methods = []
        for member in body.split(";"):
            member = " ".join(member.split())
            if not member:
                continue
            mm = re.match(r"^(.*?)\(\s*__attribute__\(\(__stdcall__\)\)\s*\*\s*(\w+)\s*\)\s*\((.*)\)$", member)
            if not mm:
                raise SystemExit(f"{name}: cannot parse vtable member: {member}")
            ret, meth, params = norm_type(mm.group(1)), mm.group(2), mm.group(3)
            plist = split_top(params)
            this = plist[0].strip()
            if not this.startswith(name):
                raise SystemExit(f"{name}::{meth}: unexpected this parameter {this!r}")
            args = [parse_param(p, i) for i, p in enumerate(plist[1:], 1)]
            methods.append((ret, meth, args))
        found.setdefault(name, methods)
    return found


def main():
    text = sys.stdin.read()
    vt = parse_vtables(text)
    lptypes = {}
    for m in re.finditer(r"typedef\s+struct\s+(\w+)\s*((?:\*+\s*\w+\s*,?\s*)+);", text):
        if m.group(1) not in INTERFACES:
            continue
        for stars, name in re.findall(r"(\*+)\s*(\w+)", m.group(2)):
            if len(stars) == 1:
                lptypes[name] = m.group(1)
    callbacks = {}
    for m in re.finditer(r"typedef\s+([\w\s\*]+?)\s*\(\s*(?:__attribute__\(\(__stdcall__\)\)|__attribute__\(\(__cdecl__\)\))?\s*\*\s*(\w+)\s*\)\s*\(([^;]*)\)\s*;", text):
        callbacks[m.group(2)] = (norm_type(m.group(1)), m.group(3), "stdcall" in m.group(0))
    print("# Generated by tools/seamrec/extract.py from the mingw-w64 DX headers. Do not edit by hand;")
    print("# per-method behaviour overrides live in gen.py (HOOKS, COUNTS).")
    used_cb = set()
    for iface in INTERFACES:
        if iface not in vt:
            raise SystemExit(f"interface {iface} not found in headers")
        has_iid = re.search(rf"\bIID_{iface}\b", text) is not None
        print(f"\ninterface {iface}{' iid' if has_iid else ''}")
        for ret, meth, args in vt[iface]:
            print(f"  {ret} {meth}({', '.join(f'{t} {n}' for t, n in args)})")
            for t, _ in args:
                if t in callbacks:
                    used_cb.add(t)
    print()
    for lp in sorted(lptypes):
        print(f"lptype {lp} {lptypes[lp]}")
    print()
    for cb in sorted(used_cb):
        ret, params, std = callbacks[cb]
        args = [parse_param(p, i) for i, p in enumerate(split_top(params), 1)] if params.strip() not in ("", "void") else []
        print(f"callback {cb} {'stdcall' if std else 'cdecl'} {ret}({', '.join(f'{t} {n}' for t, n in args)})")


if __name__ == "__main__":
    main()
