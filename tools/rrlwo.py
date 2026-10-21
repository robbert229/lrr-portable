#!/usr/bin/env python3
"""rrlwo.py — Lego Rock Raiders `.lwo` (LightWave 4.0) → glTF 2.0 (.glb).

Reads FORM/LWOB objects from disk or straight out of the game WADs (via
rrwad.py). Format knowledge is the evidence-backed set from finding 11:

    PNTS  f32 big-endian triples                    vertices (1-based)
    POLS  per poly: u16 count, count x i16 indices  (negative index = the
          polygon is wound backwards -> flip), u16 surface index (1-based)
    SRFS  NUL-terminated surface names, even-padded
    SURF  records: even-padded name + sub-blocks(tag + 1-byte size):
          COLR = r,g,b,a; TIMG = texture path (basename lives in the WAD);
          CTEX declares "Planar Image Map"; TSIZ/TCTR carry its scale/offset.
    .uv   text sidecar loaded next to the .lwo: s texture-path lines that
          override each surface's TIMG + per-polygon-corner "u v" blocks with
          v negated (finding 19; name lines and keyframe tail ignored).

glTF mapping decisions (documented approximations, per finding 11 §3):
    - LWOB stores no UVs. CTEX says the mapping is *planar*, so we project
      each face along its dominant normal axis and divide by TSIZ — the
      engine's own scheme, applied per corner. Models with a `.uv` sidecar
      instead get the engine's real per-corner UVs (v negated as stored)
      and their sidecar texture paths replace the LWO TIMG — per finding 19.
    - glTF wants per-vertex normals; LWO faces are flat, so corners sharing
      a position do NOT share a normal (keyed vertex table, not raw fan).
    - BMP payloads convert to PNG with stdlib only (struct+zlib+crc32).

Usage (repo root):
    python3 tools/rrlwo.py list [--wad-dir DIR]
    python3 tools/rrlwo.py dump PATH.lwo | -n NAME [--wad-dir DIR]
    python3 tools/rrlwo.py gltf PATH.lwo | -n NAME [-o OUT.glb]
                                 [--no-textures] [--no-uv]
    python3 tools/rrlwo.py uv   PATH.lwo | -n NAME [-o OUT.uv] [--no-uv]
                                 # rebuild sidecar; --no-uv = synthesize from model
"""
import argparse
import json
import pathlib
import struct
import sys
import zlib

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import rrwad  # noqa: E402

DEFAULT_WAD_DIRS = [
    pathlib.Path("~/.local/share/rock-raider-a2/game"),
    pathlib.Path("~/.local/share/rock-raider-a2/game").parent / "rock-raider-reversed/source",
]


def wad_dirs(explicit):
    if explicit:
        return [pathlib.Path(explicit)]
    return [d for d in DEFAULT_WAD_DIRS if d.expanduser().is_dir()]


# -- WAD-backed asset lookup -------------------------------------------------

class GameAssets:
    """basename (lower) -> (index, wad) across every *.wad found, plus exact
    full-path resolution so a pathed lookup can never land on a same-named
    file from another folder (the game resolves full paths; TIMG lookups give
    only basenames)."""

    def __init__(self, dirs=()):
        self.index = {}
        self.by_path = {}
        self.full_names = {}
        for d in dirs:
            d = pathlib.Path(d).expanduser()
            if not d.is_dir():
                continue
            for w in sorted(d.glob("*.wad")):
                wad = rrwad.Wad(w)
                for i, name in enumerate(wad.names):
                    full = name.lower().replace("\\", "/")
                    key = full.split("/")[-1]
                    self.index.setdefault(key, (i, wad))
                    self.by_path.setdefault(full, (i, wad))
                    self.full_names.setdefault(full, name)

    def read(self, name):
        full = name.lower().replace("\\", "/")
        hit = self.by_path.get(full) or self.index.get(full.split("/")[-1])
        if hit is None:
            return None
        i, wad = hit
        return wad.payload(i)

    def read_exact(self, name):
        """Path-exact only — sidecars must never fall back to a same-named
        file from another folder (`.uv` belongs to the model it sits next to)."""
        hit = self.by_path.get(name.lower().replace("\\", "/"))
        if hit is None:
            return None
        i, wad = hit
        return wad.payload(i)

    def names_with(self, suffix):
        """Full original-case names ending in `suffix` (ambiguous basenames
        would collide; the full path is what --name/-n must take back)."""
        return sorted(self.full_names[f] for f in self.full_names
                      if f.endswith(suffix.lower()))


# -- LWO container -----------------------------------------------------------

class Lwo:
    def __init__(self, data: bytes):
        if data[:4] != b"FORM" or data[8:12] != b"LWOB":
            raise ValueError("not a FORM/LWOB object")
        self.chunks = []
        end = min(len(data), 8 + struct.unpack_from(">I", data, 4)[0])
        p = 12
        while p + 8 <= end:
            tag = data[p:p + 4].decode("latin1").rstrip("\x00")
            size = struct.unpack_from(">I", data, p + 4)[0]
            self.chunks.append((tag, data[p + 8:p + 8 + size]))
            p += 8 + size + (size & 1)

    def one(self, tag):
        for t, d in self.chunks:
            if t == tag:
                return d
        return None

    def points(self):
        d = self.one("PNTS")
        if not d:
            raise ValueError("missing PNTS")
        return [struct.unpack_from(">3f", d, i) for i in range(0, len(d), 12)]

    def polys(self):
        """[(indices signed 1-based, surf_index_0based)] — finding 11 §3 POLS."""
        d = self.one("POLS")
        out, p = [], 0
        while p + 2 <= len(d):
            n = struct.unpack_from(">H", d, p)[0]
            p += 2
            if n == 0 or p + 2 * n + 2 > len(d):
                break
            idx = struct.unpack_from(f">{n}h", d, p)
            p += 2 * n + 2  # indices + per-poly surface index
            out.append((idx, max(struct.unpack_from(">h", d, p - 2)[0] - 1, 0)))
        return out

    def surface_names(self):
        d = self.one("SRFS")
        names, p = [], 0
        while d and p < len(d):
            e = d.find(b"\x00", p)
            if e < 0:
                break
            names.append(d[p:e].decode("latin1").strip())
            p = (e + 2) & ~1  # even-padded record
        return names

    def surf_records(self):
        """SURF chunks: [even-padded name][tag + u16-BE size + payload]* blocks.
        Record starts are anchored on the SRFS names themselves (SURF may
        arrive as one chunk per record); a block stream ends at the next anchor."""
        anchors = [n.encode("latin1") + b"\x00" for n in self.surface_names()]
        out, seen = [], set()

        def find_all(buf, needle):
            hits, pos = [], 0
            while (e := buf.find(needle, pos)) >= 0:
                hits.append(e)
                pos = e + 1
            return hits

        for tag, d in self.chunks:
            if tag != "SURF":
                continue
            starts = sorted({e for a in anchors for e in find_all(d, a) if e % 2 == 0})
            for k, e in enumerate(starts):
                a = next(x for x in anchors if d.startswith(x, e))
                name = a.decode("latin1").strip("\x00")
                limit = starts[k + 1] if k + 1 < len(starts) else len(d)
                q = (e + len(a) + 1) & ~1
                blocks = {}
                while q + 6 <= limit:
                    bt = d[q:q + 4]
                    if not all(0x20 <= c <= 0x7E for c in bt):
                        break
                    sz = struct.unpack_from(">H", d, q + 4)[0]
                    if sz > limit - q - 6:
                        break
                    blocks[bt.decode("latin1").strip("\x00 ")] = d[q + 6:q + 6 + sz]
                    q += 6 + sz + (sz & 1)
                if name not in seen:
                    seen.add(name)
                    out.append((name, blocks))
        return out

    def surface_meta(self):
        """name(lower) -> {colr, timg, tsiz} from SURF records."""
        meta = {}
        for name, blocks in self.surf_records():
            m = meta.setdefault(name.lower(), {})
            b = blocks.get("COLR")
            if b and len(b) >= 3:
                m["colr"] = [c / 255 for c in b[:3]] + [1.0]
            if blocks.get("TIMG"):
                m["timg"] = blocks["TIMG"].split(b"\x00")[0].decode("latin1").strip()
            s = blocks.get("TSIZ")
            if s and len(s) >= 8:
                m["tsiz"] = struct.unpack(">2f", s[:8])
        return meta

    def _index_base(self):
        """Some exporters (the 4 known here) wrote 0-based POLS despite the
        1-based spec; a zero anywhere settles it since 1-based forbids zero."""
        try:
            idx = [i for idx, _ in self.polys() for i in idx]
            return 0 if any(i == 0 for i in idx) else 1
        except Exception:
            return 1

    def triangles(self):
        """[(a, b, c, surf0, poly_idx, (corner_a, corner_b, corner_c))] with fan
        triangulation + negative-index winding. Corner indices are ring positions
        in POLS order (swapped with the winding) so `.uv` corner data lines up."""
        base = self._index_base()
        for pi, (idx, si) in enumerate(self.polys()):
            ring = [abs(i) - base for i in idx]
            rev = any(i < 0 for i in idx)
            for k in range(1, len(ring) - 1):
                ca, cb, cc = 0, k, k + 1
                b, c = ring[k], ring[k + 1]
                if rev:
                    b, c = c, b
                    cb, cc = cc, cb
                if ring[0] not in (b, c) and b != c:
                    yield ring[0], b, c, si, pi, (ca, cb, cc)


# -- .uv sidecar (finding 19) --------------------------------------------------

def parse_uv(data: bytes):
    """Parse a `.uv` sidecar per finding 19 §3. Line layout:

        1      version (ignored)            2      surface count s
        3..    s name lines (ignored)       ...    s texture paths -> TIMG override
        2+2s   polygon count m             then m blocks: "polyIdx count" +
               count lines "u v z", with v negated as the engine stores it.
        tail   keyframe blocks — ignored, like the engine's parser.

    Raises ValueError if the data does not match the grammar. Returns
    {"surfaces", "names", "paths", "polys": {poly_idx: [(u, -v), ...]}}.
    Blank lines never reach the engine's state machine (FUN_00481c50 skips
    zero-token lines), so drop them here too."""
    lines = [ln for ln in data.decode("latin1").replace("\r\n", "\n").split("\n")
             if ln.strip()]
    if len(lines) < 4:
        raise ValueError("too short to be a .uv file")
    try:
        s = int(lines[1])
    except ValueError:
        raise ValueError("missing surface-count header")
    if s < 1 or len(lines) < 2 + 2 * s + 2:
        raise ValueError("header counts inconsistent with line count")
    names = [ln.strip() for ln in lines[2:2 + s]]
    paths = [ln.strip() for ln in lines[2 + s:2 + 2 * s]]
    try:
        m = int(lines[2 + 2 * s])
    except ValueError:
        raise ValueError("missing polygon-count line")
    polys, q, expect = {}, 3 + 2 * s, 0
    while expect < m and q < len(lines):
        tok = lines[q].split()
        if len(tok) < 2:
            break
        try:
            bi, cnt = int(tok[0]), int(tok[1])
        except ValueError:
            break
        if bi != expect or q + 1 + cnt > len(lines):
            break  # tail blocks or out-of-order: engine reaches nothing either
        corners = []
        for k in range(cnt):
            t = lines[q + 1 + k].split()
            if len(t) < 2:
                raise ValueError(f"block {bi} corner {k}: expected 'u v [z]'")
            corners.append((float(t[0]), -float(t[1])))  # v negated (§4)
        polys[bi] = corners
        q += 1 + cnt
        expect += 1
    return {"surfaces": s, "names": names, "paths": paths,
            "polys": polys, "polys_header": m, "tail": lines[q:]}


def uv_sibling(name):
    """`x\\y.lwo` -> `x\\y.uv`; anything else -> None."""
    base, _, ext = name.rpartition(".")
    return base + ".uv" if ext.lower() == "lwo" else None


def write_uv(lwo, old_uv, warn=print):
    """Rebuild `.uv` text (finding 19 grammar) for this model.

    Round-trip rules, chosen to match what the engine actually consumes:
      * version `2`; header counts taken from the *model* (the engine reads
        positionally against them anyway).
      * name lines: the sidecar's own where available — they encode material
        identity (the GC_Larm pilot re-skin) — else the model's SRFS names.
      * path lines: the sidecar's own path where it had one (round-trip:
        the file is the texture truth), else a non-empty LWO TIMG, else a
        `*` placeholder and a warning. A sidecar path that shadows a
        different TIMG is reported, so the choice to adopt a LWO edit is
        explicit (`--no-uv` rebuilds from the model alone).
      * uv blocks: old per-corner values (written back in file form, i.e.
        negated v) wherever the ring size is unchanged; otherwise synthesized
        with the planar scheme. Polygons are emitted in POLS order.
      * tail: the original keyframe lines verbatim when known, else `s`
        placeholder blocks (the engine parses none of it).

    Returns (text, stats)."""
    names = lwo.surface_names()
    s = len(names)
    polys = lwo.polys()
    old_names = (old_uv["names"] if old_uv and len(old_uv["names"]) == s else None)
    old_paths = (old_uv["paths"] if old_uv and len(old_uv["paths"]) == s else None)
    old_polys = old_uv["polys"] if old_uv else None

    stats = {"surfaces": s, "polys": len(polys), "placeholders": 0,
             "names": "sidecar" if old_names else "SRFS",
             "tail": "original" if (old_uv and old_uv["tail"]) else "synthesized"}
    line = ["2", str(s)]
    for i in range(s):  # blank name lines would shift the line alignment
        nm = (old_names or names)[i]
        if not nm.strip():
            warn(f"warning: surface {i} has a blank name — wrote placeholder")
            nm = f"surface{i}"
        line.append(nm)
    meta = lwo.surface_meta()
    for i, nm in enumerate(names):
        timg = meta.get(nm.lower(), {}).get("timg")
        if old_paths and old_paths[i]:
            # sidecar path is the truth for this surface — keep it, but say so
            # when the LWO disagrees (the engine hides TIMG behind this path).
            p = old_paths[i]
            if timg and timg.lower().split("\\")[-1] != p.lower().split("\\")[-1]:
                warn(f"warning: surface {i}: LWO TIMG {timg!r} is shadowed by "
                     f"the sidecar path — rerun with --no-uv to adopt the TIMG")
        else:
            if timg:
                p = timg
            else:
                p = "*"
                stats["placeholders"] += 1
        if " " in p:
            warn(f"warning: texture path {p!r} contains whitespace — the "
                 "engine tokenizer keeps only the first token")
        line.append(p)
    line.append(str(len(polys)))

    pts = lwo.points()
    base = lwo._index_base()
    for pi, (idx, si) in enumerate(polys):
        ring = [abs(i) - base for i in idx]
        line.append(f"{pi} {len(ring)}")
        oldblk = old_polys.get(pi) if old_polys else None
        if oldblk is not None and len(oldblk) == len(ring):
            corners = oldblk
        else:  # synthesize: planar projection onto the polygon plane (finding 11 §3)
            rp = [pts[vi] if 0 <= vi < len(pts) else (0.0, 0.0, 0.0) for vi in ring]
            nx = ny = nz = 0.0
            for k in range(len(rp)):  # Newell normal over the ring
                x0, y0, z0 = rp[k]
                x1, y1, z1 = rp[(k + 1) % len(rp)]
                nx += (y0 - y1) * (z0 + z1)
                ny += (z0 - z1) * (x0 + x1)
                nz += (x0 - x1) * (y0 + y1)
            axis = max(range(3), key=lambda t: abs((nx, ny, nz)[t]))
            us, vs = [t for t in range(3) if t != axis]
            sx, sy = (meta.get(names[si].lower(), {}) if si < len(names) else {}
                      ).get("tsiz", (1.0, 1.0))
            corners = [(rp[k][us] / (sx or 1), rp[k][vs] / (sy or 1))
                       for k in range(len(ring))]
        for eu, ev in corners:
            vfile = -ev if -ev != 0 else 0.0
            line.append(f"{eu:.6f} {vfile:.6f} 0")

    line.extend(old_uv["tail"] if (old_uv and old_uv["tail"]) else
                (["1", "0 0 0 0 0 0"] * s if s else ["0"]))
    return "\r\n".join(line) + "\r\n", stats


# -- BMP → PNG (stdlib only) -------------------------------------------------

def bmp_to_png(data: bytes):
    if len(data) < 26 or data[:2] != b"BM":
        return None
    try:
        doff = struct.unpack_from("<I", data, 10)[0]
        hsz = struct.unpack_from("<I", data, 14)[0]
        if hsz >= 40:
            w, h = struct.unpack_from("<ii", data, 18)
            bpp = struct.unpack_from("<H", data, 26)[0]
            colors = struct.unpack_from("<I", data, 32)[0]
            psz = 4
        elif hsz >= 12:
            w, h = struct.unpack_from("<hh", data, 18)
            bpp = struct.unpack_from("<H", data, 24)[0]
            colors, psz = 0, 3
        else:
            return None
    except (struct.error, IndexError):
        return None
    flip = h > 0
    h = abs(h)
    if not (1 <= w <= 8192 and 1 <= h <= 8192) or bpp not in (1, 4, 8, 24, 32):
        return None
    palette = []
    if bpp <= 8:
        n = colors or (1 << bpp)
        for i in range(n):
            off = 14 + hsz + i * psz
            if off + 2 >= len(data):
                break
            b, g, r = data[off], data[off + 1], data[off + 2]
            palette.append(bytes((r, g, b, 255)))
        if not palette:
            palette = [bytes((i, i, i, 255)) for i in range(1 << bpp)]
    pal = b"".join(palette)
    npal = len(palette)
    stride = ((w * bpp + 31) // 32) * 4
    raw = bytearray()
    for y in range(h):
        row_off = doff + ((h - 1 - y) if flip else y) * stride
        row = data[row_off:row_off + stride]
        if len(row) < stride:
            return None
        raw.append(0)
        if bpp == 8:
            raw.extend(b"".join(pal[4 * i:4 * i + 4] for i in (row[x] % npal for x in range(w))))
        elif bpp == 4:
            nib = [((row[x // 2] >> (4 if x % 2 == 0 else 0)) & 15) % npal for x in range(w)]
            raw.extend(b"".join(pal[4 * i:4 * i + 4] for i in nib))
        elif bpp == 1:
            bit = [((row[x // 8] >> (7 - x % 8)) & 1) % npal for x in range(w)]
            raw.extend(b"".join(pal[4 * i:4 * i + 4] for i in bit))
        elif bpp == 24:
            raw.extend(b"".join(bytes((row[3 * x + 2], row[3 * x + 1], row[3 * x], 255))
                                for x in range(w)))
        else:  # 32
            raw.extend(b"".join(bytes((row[4 * x + 2], row[4 * x + 1], row[4 * x], 255))
                                for x in range(w)))
    return png_encode(w, h, bytes(raw))


def png_encode(w, h, raw):
    def chunk(tag, body):
        return (struct.pack(">I", len(body)) + tag + body
                + struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF))
    return (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(raw, 6))
            + chunk(b"IEND", b""))


# -- glTF/GLB build ----------------------------------------------------------

class Glb:
    def __init__(self):
        self.data = bytearray()
        self.views = []
        self.accessors = []
        self.images = []
        self.textures = []
        self.materials = []
        self.primitives = []

    def add_view(self, blob, target=None):
        off = len(self.data)
        self.data += blob
        pad = (-len(self.data)) % 4
        self.data += b"\x00" * pad
        view = {"buffer": 0, "byteOffset": off, "byteLength": len(blob)}
        if target:
            view["target"] = target
        self.views.append(view)
        return len(self.views) - 1

    def add_accessor(self, view, ctype, vtype, count, minmax=None):
        acc = {"bufferView": view, "componentType": ctype, "count": count, "type": vtype}
        if minmax:
            acc["min"], acc["max"] = minmax
        self.accessors.append(acc)
        return len(self.accessors) - 1

    def finish(self):
        doc = {
            "asset": {"version": "2.0", "generator": "rrlwo.py (LegoRR LWOB reader)"},
            "scene": 0,
            "scenes": [{"nodes": [0]}],
            "nodes": [{"mesh": 0, "name": "model"}],
            "meshes": [{"primitives": self.primitives}],
            "materials": self.materials,
            "accessors": self.accessors,
            "bufferViews": self.views,
            "buffers": [{"byteLength": len(self.data)}],
        }
        if self.images:
            doc["images"] = self.images
            doc["textures"] = self.textures
        blob = bytes(self.data)
        js = json.dumps(doc, separators=(",", ":")).encode()
        js += b" " * (-len(js) % 4)
        return (struct.pack("<4sII", b"glTF", 2, 12 + 8 + len(js) + 8 + len(blob))
                + struct.pack("<I4s", len(js), b"JSON") + js
                + struct.pack("<I4s", len(blob), b"BIN\x00") + blob)


def build_glb(lwo, assets, with_textures=True, warn=print, uv=None):
    glb = Glb()
    pts = lwo.points()
    names = lwo.surface_names()
    meta = lwo.surface_meta()

    # .uv sidecar semantics (finding 19): the path lines replace each surface's
    # TIMG name — the engine's own override — and the blocks give per-corner UV.
    uv_polys = {}
    if uv:
        if uv["surfaces"] != len(names):
            warn(f"warning: .uv header says {uv['surfaces']} surfaces, model has "
                 f"{len(names)} — misaligned sidecar")
        if uv["polys_header"] != len(lwo.polys()):
            warn(f"warning: .uv header says {uv['polys_header']} polys, model has "
                 f"{len(lwo.polys())} — misaligned sidecar")
        for i, p in enumerate(uv["paths"][:len(names)]):
            if p:
                meta.setdefault(names[i].lower(), {})["timg"] = p
        uv_polys = uv["polys"]

    # keyed corner vertices per surface: flat normals + planar projection uv
    surf_verts = {}  # surf0 -> {corner key: vi}
    surf_pos, surf_nrm, surf_uv, surf_idx = {}, {}, {}, {}
    for a, b, c, si, pi, corners in lwo.triangles():
        if max(a, b, c) >= len(pts) or min(a, b, c) < 0:
            warn("warning: vertex index out of range — skipping face")
            continue
        p0, p1, p2 = pts[a], pts[b], pts[c]
        u = [p1[i] - p0[i] for i in range(3)]
        v = [p2[i] - p0[i] for i in range(3)]
        n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
        m = sum(x * x for x in n) ** 0.5
        if m == 0:
            continue
        n = [x / m for x in n]
        axis = max(range(3), key=lambda k: abs(n[k]))
        us, vs = [i for i in range(3) if i != axis]
        meta_m = meta.get(names[si].lower(), {}) if si < len(names) else {}
        sx, sy = meta_m.get("tsiz", (1.0, 1.0))
        puvs = uv_polys.get(pi)
        table = surf_verts.setdefault(si, {})
        pos, nrm, uv_ = (surf_pos.setdefault(si, []), surf_nrm.setdefault(si, []),
                         surf_uv.setdefault(si, []))
        tri = []
        for p, ci in zip((p0, p1, p2), corners):
            # sidecar UV wins where present (the vertex-duplication seam break is
            # then automatic: same position + different uv keys as a new vertex)
            fu = puvs[ci] if puvs and ci < len(puvs) else (p[us] / (sx or 1),
                                                           p[vs] / (sy or 1))
            key = (p, tuple(n), fu)
            vi = table.get(key)
            if vi is None:
                vi = len(pos)
                table[key] = vi
                pos.append(p)
                nrm.append(tuple(n))
                uv_.append(fu)
            tri.append(vi)
        surf_idx.setdefault(si, []).extend(tri)

    # primitives: vertices per surface via the key table, real face indices
    tex_cache = {}
    for si in sorted(surf_pos):
        pos, nrm, uv = surf_pos[si], surf_nrm[si], surf_uv[si]
        idx = surf_idx[si]
        n = len(pos)
        if n == 0 or not idx:
            continue
        mn = [min(p[k] for p in pos) for k in range(3)]
        mx = [max(p[k] for p in pos) for k in range(3)]
        pv = glb.add_view(struct.pack(f"<{3 * n}f", *[c for p in pos for c in p]))
        nv = glb.add_view(struct.pack(f"<{3 * n}f", *[c for p in nrm for c in p]))
        uvv = glb.add_view(struct.pack(f"<{2 * n}f", *[c for v in uv for c in v]))
        ityp = 5125 if n >= 65536 else 5123
        iblob = (struct.pack(f"<{len(idx)}I", *idx) if ityp == 5125
                 else struct.pack(f"<{len(idx)}H", *idx))
        iv = glb.add_view(iblob)
        pa = glb.add_accessor(pv, 5126, "VEC3", n, (mn, mx))
        na = glb.add_accessor(nv, 5126, "VEC3", n)
        ua = glb.add_accessor(uvv, 5126, "VEC2", n)
        ia = glb.add_accessor(iv, ityp, "SCALAR", len(idx))
        mat = {"pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 0.7},
               "doubleSided": True, "name": names[si] if si < len(names) else f"surf{si}"}
        m = meta.get((names[si] if si < len(names) else "").lower(), {})
        if m.get("colr"):
            mat["pbrMetallicRoughness"]["baseColorFactor"] = m["colr"]
        if with_textures and m.get("timg"):
            key = m["timg"].lower().split("/")[-1]
            if key not in tex_cache:
                data = assets.read(key)
                tex_cache[key] = (bmp_to_png(data) if data else None, bool(data))
            png, found = tex_cache[key]
            if png:
                iv2 = glb.add_view(png)
                glb.images.append({"bufferView": iv2, "mimeType": "image/png"})
                glb.textures.append({"source": len(glb.images) - 1})
                mat["pbrMetallicRoughness"]["baseColorTexture"] = {"index": len(glb.textures) - 1}
            elif found is False:
                warn(f"warning: texture {key} not found in wads")
            elif png is None and found:
                warn(f"warning: texture {key} not decodable (not a BMP?)")
        glb.materials.append(mat)
        glb.primitives.append({
            "attributes": {"POSITION": pa, "NORMAL": na, "TEXCOORD_0": ua},
            "indices": ia, "material": len(glb.materials) - 1})
    if not glb.primitives:
        raise ValueError("no faces (point-only polygons — particle sprite)"
                         if lwo.polys() else "no geometry")
    return glb.finish()


# -- cli ---------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(description="LegoRR .lwo (LightWave 4) tools")
    ap.add_argument("cmd", choices=["list", "dump", "gltf", "uv"])
    ap.add_argument("target", nargs="?", help="path to .lwo on disk")
    ap.add_argument("-n", "--name", help="resolve inside the WADs instead")
    ap.add_argument("--wad-dir", action="append", dest="wad_dirs")
    ap.add_argument("-o", "--output")
    ap.add_argument("--no-textures", action="store_true")
    ap.add_argument("--no-uv", action="store_true",
                    help="ignore the .uv sidecar (planar UV, LWO texture only)")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args(argv)

    dirs = args.wad_dirs or wad_dirs(None)
    assets = GameAssets(dirs)
    if args.cmd == "list":
        for n in assets.names_with(".lwo"):
            print(n)
        return 0

    if args.name:
        data = assets.read(args.name)
        if data is None:
            ap.error(f"{args.name!r} not found in wads: {dirs}")
    elif args.target:
        data = pathlib.Path(args.target).read_bytes()
    else:
        ap.error("need PATH or --name NAME")

    warn = (lambda *_: None) if args.quiet else print
    uv = None
    if not args.no_uv:
        uvn = uv_sibling(args.name or args.target)
        blob = assets.read_exact(uvn) if (uvn and args.name) else None
        if blob is None and uvn and args.target:
            sib = pathlib.Path(args.target).with_suffix(".uv")
            blob = (sib.read_bytes() if sib.is_file()
                    else assets.read_exact(uvn))
        if blob is not None:
            try:
                uv = parse_uv(blob)
            except ValueError as e:
                warn(f"warning: .uv sidecar not understood ({e}) — exporting without it")

    lwo = None
    try:
        lwo = Lwo(data)
    except ValueError:
        ap.error("not a FORM/LWOB object — pass a .lwo (or wad entry) as target")
    if args.cmd == "dump":
        pts = lwo.points()
        print(f"chunks: {[(t, len(d)) for t, d in lwo.chunks]}")
        tris = list(lwo.triangles())
        print(f"points={len(pts)} polys={len(lwo.polys())} tris={len(tris)} "
              f"surfaces={lwo.surface_names()}")
        if uv:
            print(f"uv: surfaces={uv['surfaces']} polys={len(uv['polys'])} "
                  f"(v negated as stored; tail keyframes ignored)")
            for i, p in enumerate(uv["paths"]):
                print(f"  tex[{i}] {p}")
        for name, blocks in lwo.surf_records():
            t = blocks.get("TIMG")
            print(f"  SURF {name!r}: tags={sorted(blocks)} "
                  f"timg={t.split(b'.')[0].decode(errors='replace') if t else None}")
        return 0

    if args.cmd == "uv":
        text, stats = write_uv(lwo, uv, warn)
        base = (args.name or args.target).rsplit(".", 1)[0].replace("\\", "/").split("/")[-1]
        out = pathlib.Path(args.output or base + ".uv")
        blob = text.encode("latin1")
        out.write_bytes(blob)
        extra = f", {stats['placeholders']} '*' placeholder path(s)" if stats["placeholders"] else ""
        print(f"wrote {out} ({len(blob):,} bytes): "
              f"surfaces={stats['surfaces']} polys={stats['polys']} "
              f"names={stats['names']} tail={stats['tail']}{extra}")
        return 0

    glb = build_glb(lwo, assets, not args.no_textures, warn, uv)
    base = (args.name or args.target).rsplit(".", 1)[0].replace("\\", "/").split("/")[-1]
    out = pathlib.Path(args.output or base + ".glb")
    out.write_bytes(glb)
    note = f", uv sidecar applied ({uv['surfaces']} surfaces)" if uv else ""
    print(f"wrote {out} ({len(glb):,} bytes{note})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
