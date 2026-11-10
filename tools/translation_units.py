#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Recover the original translation units of LegoRR.exe (issue #383).

The shipped executable was linked by MSVC 5.0 from C objects in project
order, followed by the gods98 engine, the static CRT and import libraries.
Each object contributes one contiguous run to every output section, so the
order of code in .text matches the order of each object's .rdata, .data and
.bss contributions.  This tool extracts that evidence from the reference
binary, proposes unit boundaries, and validates the curated unit list in
``docs/coverage/translation-units.spec.csv``.

Commands
  evidence   print layout constants and summary statistics
  propose    print a cohesion-DP segmentation (method input only)
  dossier A B
             print per-function evidence for a text address range, with the
             current spec's assignment, for reviewing boundaries
  build      validate the spec and write the per-function map and the
             per-unit range table under docs/coverage/
  check      validate only; non-zero exit on structural errors

Requires the Python packages ``pefile`` and ``capstone``.  The reference
binary must be ``reference/LegoRR.exe`` (SHA-256 c3d8ce23...) or be given
with ``--exe``.
"""

from __future__ import annotations

import argparse
import bisect
import collections
import csv
import hashlib
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "docs/decompiles/full/manifest.csv"
LEDGER = ROOT / "docs/coverage/ledger.csv"
AREAS = ROOT / "docs/coverage/function-areas.csv"
SPEC = ROOT / "docs/coverage/translation-units.spec.csv"
OUT_FUNCS = ROOT / "docs/coverage/translation-units.csv"
OUT_UNITS = ROOT / "docs/coverage/translation-unit-ranges.csv"
DEFAULT_EXE = ROOT / "reference/LegoRR.exe"
EXPECTED_SHA256 = "c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f"

# Section layout of the reference binary.  Derived once from the PE headers
# and the reference pattern (see finding 505) and re-checked on load.
RDATA = (0x0049E000, 0x004A1000)
DATA_INIT = (0x004A1000, 0x004B2DB0)   # initialised .data: tables + literals
BSS_OBJ = (0x004B2DB0, 0x00549520)     # per-object .bss in link order
COMMON = (0x00549520, 0x0076E000)      # C common (tentative) definitions
REGIONS = ("rdata", "string", "data", "bss")
FIRST_USE = {"rdata", "string"}        # COMDAT/pooled: placed with first user

IDENT = re.compile(r"[A-Za-z][A-Za-z0-9]*_[A-Za-z0-9_]+")


# --------------------------------------------------------------------------
# Binary evidence


class Binary:
    def __init__(self, path: pathlib.Path):
        import pefile

        data = path.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest != EXPECTED_SHA256:
            raise SystemExit(f"{path}: SHA-256 {digest} is not the reference binary")
        self.pe = pefile.PE(data=data)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.image = self.pe.get_memory_mapped_image()
        text = next(s for s in self.pe.sections if s.Name.rstrip(b"\0") == b".text")
        self.text = (self.base + text.VirtualAddress,
                     self.base + text.VirtualAddress + text.Misc_VirtualSize)
        data_sec = next(s for s in self.pe.sections if s.Name.rstrip(b"\0") == b".data")
        raw_end = self.base + data_sec.VirtualAddress + data_sec.SizeOfRawData
        last = raw_end
        while last > DATA_INIT[0] and self.read(last - 1, 1) == b"\0":
            last -= 1
        if (last + 15) & ~15 != DATA_INIT[1]:
            raise SystemExit("unexpected end of initialised .data")

    def read(self, va: int, size: int) -> bytes:
        off = va - self.base
        return bytes(self.image[off:off + size])

    def cstring(self, va: int, limit: int = 256) -> str | None:
        if not DATA_INIT[0] <= va < DATA_INIT[1]:
            return None
        raw = self.read(va, limit)
        end = raw.find(b"\0")
        if end < 1:
            return None
        body = raw[:end]
        if all(32 <= c < 127 or c in (9, 10, 13) for c in body):
            return body.decode("ascii")
        return None


def region_of(va: int, strings: set[int]) -> str | None:
    if RDATA[0] <= va < RDATA[1]:
        return "rdata"
    if DATA_INIT[0] <= va < DATA_INIT[1]:
        return "string" if va in strings else "data"
    if BSS_OBJ[0] <= va < BSS_OBJ[1]:
        return "bss"
    if COMMON[0] <= va < COMMON[1]:
        return "common"
    return None


class Function:
    __slots__ = ("index", "address", "name", "status", "area", "end", "next",
                 "pad", "refs", "calls")

    def __init__(self, index, address, name, status, area):
        self.index = index
        self.address = address
        self.name = name
        self.status = status
        self.area = area
        self.refs: list[int] = []
        self.calls: list[int] = []

    @property
    def fill(self) -> str:
        kinds = set(self.pad)
        if not self.pad:
            return "none"
        if kinds == {0x90}:
            return "nop"
        if kinds == {0xCC}:
            return "int3"
        return "other"


def read_rows(path: pathlib.Path) -> list[dict]:
    with path.open(newline="", encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


def load_functions(binary: Binary) -> list[Function]:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM

    ledger = {int(r["address"], 16): r for r in read_rows(LEDGER)}
    areas = {int(r["address"], 16): r for r in read_rows(AREAS)}
    addrs = sorted(int(r["address"], 16) for r in read_rows(MANIFEST))
    funcs = []
    for i, a in enumerate(addrs):
        row = ledger[a]
        funcs.append(Function(i, a, row["corpus_name"] or row["symbol"], row["status"],
                              areas[a]["area_id"]))

    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    lo, hi = RDATA[0], COMMON[1]
    for i, f in enumerate(funcs):
        nxt = funcs[i + 1].address if i + 1 < len(funcs) else binary.text[1]
        code = binary.read(f.address, nxt - f.address)
        seen: set[int] = set()
        work = [f.address]
        end = f.address
        while work:
            a = work.pop()
            while f.address <= a < nxt and a not in seen:
                window = code[a - f.address:a - f.address + 16]
                ins = next(md.disasm(window, a), None)
                if ins is None:
                    break
                seen.add(a)
                end = max(end, a + ins.size)
                for op in ins.operands:
                    v = None
                    if op.type == X86_OP_IMM:
                        v = op.imm & 0xFFFFFFFF
                    elif op.type == X86_OP_MEM:
                        v = op.mem.disp & 0xFFFFFFFF
                    if v is not None and lo <= v < hi:
                        f.refs.append(v)
                m = ins.mnemonic
                if m == "call" and ins.operands and ins.operands[0].type == X86_OP_IMM:
                    f.calls.append(ins.operands[0].imm & 0xFFFFFFFF)
                if m.startswith("j") and ins.operands and ins.operands[0].type == X86_OP_IMM:
                    t = ins.operands[0].imm
                    if f.address <= t < nxt:
                        work.append(t)
                    if m == "jmp":
                        break
                elif m == "jmp":
                    op = ins.operands[0] if ins.operands else None
                    if op is not None and op.type == X86_OP_MEM and \
                            f.address <= op.mem.disp < nxt:
                        p = op.mem.disp       # in-function jump table
                        while f.address <= p < nxt:
                            t = int.from_bytes(binary.read(p, 4), "little")
                            if not f.address <= t < nxt:
                                break
                            work.append(t)
                            p += 4
                    break
                if m in ("ret", "retn", "int3", "hlt"):
                    break
                a += ins.size
        f.end = end
        f.next = nxt
        f.pad = binary.read(end, nxt - end)
    return funcs


class Evidence:
    """Per-function references classified by data region."""

    def __init__(self, binary: Binary, funcs: list[Function]):
        self.binary = binary
        self.funcs = funcs
        self.by_addr = {f.address: f for f in funcs}
        self.strings = {v for f in funcs for v in f.refs if binary.cstring(v) is not None}
        self.refs: list[dict[str, set[int]]] = []
        self.refby: dict[int, set[int]] = collections.defaultdict(set)
        for f in funcs:
            per = collections.defaultdict(set)
            for v in f.refs:
                r = region_of(v, self.strings)
                if r:
                    per[r].add(v)
                    self.refby[v].add(f.index)
            self.refs.append(per)
        # First user owns COMDAT/pooled items.
        self.owner = {v: min(s) for v, s in self.refby.items()}
        self.callers: dict[int, set[int]] = collections.defaultdict(set)
        for f in funcs:
            for t in f.calls:
                g = self.by_addr.get(t)
                if g is not None and g.index != f.index:
                    self.callers[g.index].add(f.index)

    def region_items(self, region: str) -> list[int]:
        return sorted(v for v in self.refby if region_of(v, self.strings) == region)

    def file_anchor(self, i: int) -> str | None:
        for v in self.refs[i].get("string", ()):
            s = self.binary.cstring(v)
            if s and s.startswith("C:\\Dev\\"):
                return s.rsplit("\\", 1)[-1]
        return None

    def owned(self, i: int, region: str) -> set[int]:
        items = self.refs[i].get(region, set())
        if region in FIRST_USE:
            return {v for v in items if self.owner[v] == i}
        return items


# --------------------------------------------------------------------------
# Proposal: contiguous modularity segmentation of a cohesion graph


def propose(ev: Evidence, gamma: float) -> list[int]:
    import numpy as np

    n = len(ev.funcs)
    w = np.zeros((n, n))

    def clique(members, weight):
        members = sorted(members)
        if len(members) < 2:
            return
        share = weight / (len(members) - 1)
        for a in members:
            for b in members:
                if a != b:
                    w[a, b] += share

    weight = {"rdata": 0.5, "string": 0.3, "data": 1.0, "bss": 1.0, "common": 1.0}
    bucket = {"rdata": 8, "string": 4, "data": 32, "bss": 32, "common": 64}
    groups = collections.defaultdict(set)
    for v, users in ev.refby.items():
        r = region_of(v, ev.strings)
        groups[(r, v // bucket[r])] |= users
    for (r, _), users in groups.items():
        clique(users, weight[r])
    for r in REGIONS:
        items = ev.region_items(r)
        for a, b in zip(items, items[1:]):
            if b - a > 64:
                continue
            sa = {ev.owner[a]} if r in FIRST_USE else ev.refby[a]
            sb = {ev.owner[b]} if r in FIRST_USE else ev.refby[b]
            share = 0.5 / (len(sa) * len(sb))
            for p in sa:
                for q in sb:
                    if p != q:
                        w[p, q] += share
                        w[q, p] += share
    for callee, callers in ev.callers.items():
        share = 1.0 / len(callers)
        for c in callers:
            w[c, callee] += share
            w[callee, c] += share

    deg = w.sum(1)
    total = w.sum() or 1.0
    cum = np.zeros((n + 1, n + 1))
    cum[1:, 1:] = w.cumsum(0).cumsum(1)
    dcum = np.concatenate([[0.0], deg.cumsum()])
    best = np.full(n + 1, -1e18)
    best[0] = 0.0
    arg = np.zeros(n + 1, dtype=int)
    for j in range(1, n + 1):
        i = np.arange(j)
        inner = cum[j, j] - cum[i, j] - cum[j, i] + cum[i, i]
        score = best[:j] + inner / total - gamma * ((dcum[j] - dcum[i]) / total) ** 2 - 1e-9
        k = int(np.argmax(score))
        best[j] = score[k]
        arg[j] = k
    cuts = []
    j = n
    while j > 0:
        cuts.append(int(arg[j]))
        j = int(arg[j])
    return sorted(cuts)


def source_markers() -> dict[int, str]:
    """Map entry address -> src/ file carrying its Reccmp marker."""
    marker = re.compile(r"//\s*(?:FUNCTION|LIBRARY|STUB):\s+LEGORR\s+0x([0-9a-fA-F]{1,8})\b")
    found: dict[int, str] = {}
    for path in sorted((ROOT / "src").rglob("*")):
        if path.suffix not in (".c", ".cpp", ".h", ".hpp") or path.name.startswith("test_"):
            continue
        rel = str(path.relative_to(ROOT))
        for m in marker.finditer(path.read_text(encoding="utf-8", errors="replace")):
            found.setdefault(int(m.group(1), 16), rel)
    return found


# --------------------------------------------------------------------------
# Spec validation


class Unit:
    def __init__(self, row: dict, ordinal: int):
        self.ordinal = ordinal
        self.start = int(row["start"], 16)
        self.id = row["unit_id"]
        self.name = row["name"]
        self.kind = row["kind"]
        self.name_basis = row["name_basis"]
        self.note = row["note"]
        self.first = 0
        self.last = 0
        self.ranges: dict[str, tuple[int, int] | None] = {}
        self.anchors: list[int] = []
        self.boundary = {}


def load_spec() -> list[Unit]:
    rows = read_rows(SPEC)
    rows = [r for r in rows if not r["start"].startswith("#")]
    units = [Unit(r, k) for k, r in enumerate(rows)]
    ids = collections.Counter(u.id for u in units)
    dup = [i for i, c in ids.items() if c > 1]
    if dup:
        raise SystemExit(f"duplicate unit ids in spec: {dup}")
    return units


def assign(units: list[Unit], ev: Evidence) -> list[int]:
    starts = [u.start for u in units]
    if starts != sorted(starts) or len(set(starts)) != len(starts):
        raise SystemExit("spec starts are not strictly increasing")
    unit_of = []
    for f in ev.funcs:
        k = bisect.bisect_right(starts, f.address) - 1
        if k < 0:
            raise SystemExit(f"function {f.address:08x} precedes the first unit")
        unit_of.append(k)
    by_addr = {f.address for f in ev.funcs}
    for u in units:
        if u.start not in by_addr:
            raise SystemExit(f"unit {u.id} start {u.start:08x} is not a function address")
    for k, u in enumerate(units):
        members = [i for i, uk in enumerate(unit_of) if uk == k]
        u.first, u.last = members[0], members[-1]
    return unit_of


def home_units(ev: Evidence, unit_of: list[int], region: str, nunits: int):
    """Monotone assignment of each region item to a unit.

    Maximises the number of items whose home unit is one of their users
    (first user only for COMDAT/pooled regions) subject to home units being
    non-decreasing in address, i.e. link order.  Returns (items, homes,
    matched flags).
    """
    items = ev.region_items(region)
    users = []
    for v in items:
        if region in FIRST_USE:
            users.append({unit_of[ev.owner[v]]})
        else:
            users.append({unit_of[i] for i in ev.refby[v]})
    neg = -1 << 30
    score = [0] * nunits
    choice = []
    for us in users:
        run = neg
        best_k = []
        new = [0] * nunits
        arg = 0
        for k in range(nunits):
            if score[k] > run:
                run = score[k]
                arg = k
            new[k] = run + (1 if k in us else 0)
            best_k.append(arg)
        score = new
        choice.append(best_k)
    k = max(range(nunits), key=lambda x: (score[x], -x))
    homes = [0] * len(items)
    for j in range(len(items) - 1, -1, -1):
        homes[j] = k
        k = choice[j][k]
    matched = [homes[j] in users[j] for j in range(len(items))]
    # Unmatched items inherit the previous matched home (keeps runs tidy).
    last = homes[0] if homes else 0
    for j in range(len(items)):
        if matched[j]:
            last = homes[j]
        else:
            homes[j] = last
    return items, homes, matched


def padding_regime(f: Function) -> str:
    aligned = f.address % 16 == 0
    return f"{'a16' if aligned else 'packed'}:{f.fill}"


def packed(ev: Evidence, unit: Unit) -> bool:
    """True when the unit's functions are not 16-byte aligned (MASM or
    differently compiled object); C objects align every function."""
    return any(ev.funcs[i].address % 16 for i in range(unit.first + 1, unit.last + 1))


def analyse(units: list[Unit], ev: Evidence, unit_of: list[int]):
    n = len(units)
    item_home = {}
    for region in REGIONS:
        items, homes, matched = home_units(ev, unit_of, region, n)
        spans = collections.defaultdict(list)
        for v, h, ok in zip(items, homes, matched):
            item_home[(region, v)] = (h, ok)
            if ok:
                spans[h].append(v)
        for k, u in enumerate(units):
            vs = spans.get(k)
            u.ranges[region] = (min(vs), max(vs)) if vs else None

    anchored = [set() for _ in ev.funcs]   # regions anchoring function to its unit
    conflicts = [[] for _ in ev.funcs]
    for f in ev.funcs:
        k = unit_of[f.index]
        if ev.file_anchor(f.index):
            anchored[f.index].add("file")
        for region in REGIONS:
            for v in ev.owned(f.index, region):
                h, ok = item_home[(region, v)]
                if not ok:
                    continue
                if h == k:
                    anchored[f.index].add(region)
                elif region in FIRST_USE:
                    conflicts[f.index].append((region, v, h))
    for k, u in enumerate(units):
        u.anchors = [i for i in range(u.first, u.last + 1) if anchored[i]]

    for k in range(n - 1):
        left, right = units[k], units[k + 1]
        lf, rf = ev.funcs[left.last], ev.funcs[right.first]
        kinds = []
        if lf.fill == "int3" or packed(ev, left) != packed(ev, right):
            kinds.append("padding")
        if (lf.status in ("runtime-import", "runtime-library")) != \
                (rf.status in ("runtime-import", "runtime-library")):
            kinds.append("library")
        if left.kind != right.kind:
            kinds.append("segment")
        lfiles = {ev.file_anchor(i) for i in range(left.first, left.last + 1)} - {None}
        rfiles = {ev.file_anchor(i) for i in range(right.first, right.last + 1)} - {None}
        if lfiles and rfiles and lfiles != rfiles:
            kinds.append("file")
        # Both sides carry public symbols of different known library source files.
        if left.name_basis == right.name_basis == "library-symbol":
            kinds.append("library-symbol")
        steps = []
        for region in REGIONS:
            a, b = left.ranges.get(region), right.ranges.get(region)
            if a and b and a[1] < b[0]:
                steps.append(f"{region}@{b[0]:08x}")
        a_idx = left.anchors[-1] if left.anchors else None
        b_idx = right.anchors[0] if right.anchors else None
        slack = None
        if a_idx is not None and b_idx is not None:
            slack = b_idx - a_idx - 1
        # Call-locality for unanchored functions in the slack zone.
        votes = []
        if slack:
            for i in range(a_idx + 1, b_idx):
                lv = rv = 0
                f = ev.funcs[i]
                neigh = set(ev.callers.get(i, ())) | {ev.by_addr[t].index for t in f.calls
                                                     if t in ev.by_addr}
                for j in neigh:
                    if left.first <= j <= left.last and anchored[j]:
                        lv += 1
                    elif right.first <= j <= right.last and anchored[j]:
                        rv += 1
                votes.append((i, lv, rv))
        consistent = all((i < right.first and lv >= rv) or (i >= right.first and rv >= lv)
                         for i, lv, rv in votes)
        decided = all(lv != rv for _, lv, rv in votes)
        structural = {"padding", "library", "segment", "file"} & set(kinds)
        if structural and (slack in (None, 0) or "library" in kinds or "segment" in kinds
                           or "padding" in kinds):
            conf = "high"
        elif steps and slack == 0:
            conf = "high"
        elif steps and slack is not None and consistent and decided:
            conf = "medium"
        elif steps and slack is not None and slack <= 2 and consistent:
            conf = "medium"
        else:
            conf = "low"
        if conf == "low" and "library-symbol" in kinds:
            conf = "medium"
        right.boundary = {"kinds": kinds + (["data-step"] if steps else []), "steps": steps,
                          "slack": slack, "votes": votes, "confidence": conf}
    units[0].boundary = {"kinds": ["image-start"], "steps": [], "slack": 0, "votes": [],
                         "confidence": "high"}

    fconf = []
    for f in ev.funcs:
        k = unit_of[f.index]
        u = units[k]
        if anchored[f.index]:
            c = "high"
        elif u.anchors and u.anchors[0] < f.index < u.anchors[-1]:
            c = "high"
        elif u.anchors and f.index < u.anchors[0]:
            c = u.boundary["confidence"]
        elif u.anchors and f.index > u.anchors[-1]:
            c = units[k + 1].boundary["confidence"] if k + 1 < n else "high"
        else:
            c = min(u.boundary["confidence"],
                    units[k + 1].boundary["confidence"] if k + 1 < n else "high",
                    key=["low", "medium", "high"].index)
        if conflicts[f.index]:
            c = "low"
        fconf.append(c)
    return anchored, conflicts, fconf


# --------------------------------------------------------------------------
# Commands


def cmd_evidence(ev: Evidence):
    funcs = ev.funcs
    fills = collections.Counter(f.fill for f in funcs)
    aligned = sum(1 for f in funcs if f.address % 16 == 0)
    print(f"functions: {len(funcs)}  16-byte aligned: {aligned}  fill: {dict(fills)}")
    for region in REGIONS:
        items = ev.region_items(region)
        seq = [ev.owner[v] for v in items]
        tails = []
        for x in seq:
            p = bisect.bisect_right(tails, x)
            if p == len(tails):
                tails.append(x)
            else:
                tails[p] = x
        print(f"{region:7} items {len(items):5}  first-user order kept by "
              f"{len(tails) / max(1, len(items)):.1%}")


def cmd_propose(ev: Evidence, gamma: float):
    cuts = propose(ev, gamma)
    cuts.append(len(ev.funcs))
    for k in range(len(cuts) - 1):
        a, b = cuts[k], cuts[k + 1]
        f = ev.funcs[a]
        print(f"{f.address:08x},{b - a},{f.name}")


def describe_strings(ev: Evidence, i: int, limit: int = 3) -> str:
    owned = sorted(ev.owned(i, "string"))
    if not owned:
        return ""
    texts = [ev.binary.cstring(v) for v in owned]
    idents = [t for t in texts if IDENT.fullmatch(t)]
    prefixes = collections.Counter(t.split("_")[0] for t in idents)
    sample = [t for t in texts if not IDENT.fullmatch(t)][:limit]
    out = f"S{len(owned)}@{owned[0]:x}"
    if prefixes:
        out += " " + ",".join(f"{p}_*{c}" for p, c in prefixes.most_common(3))
    if sample:
        out += " " + " | ".join(repr(s[:28]) for s in sample)
    return out


def cmd_dossier(ev: Evidence, lo: int, hi: int):
    units = None
    unit_of = None
    anchored = None
    if SPEC.exists():
        units = load_spec()
        unit_of = assign(units, ev)
        anchored, _, _ = analyse(units, ev, unit_of)
    for f in ev.funcs:
        if not lo <= f.address < hi:
            continue
        i = f.index
        tag = ""
        if units is not None:
            u = units[unit_of[i]]
            mark = "*" if u.first == i else " "
            tag = f"{mark}{u.id:>5} {','.join(sorted(anchored[i])):18}"
        r = ev.refs[i]
        bss = sorted(r.get("bss", ()))
        data = sorted(r.get("data", ()))
        com = sorted(r.get("common", ()))
        rdat = sorted(ev.owned(i, "rdata"))
        parts = []
        if bss:
            parts.append(f"B{bss[0]:x}..{bss[-1]:x}({len(bss)})")
        if data:
            parts.append(f"D{data[0]:x}..{data[-1]:x}({len(data)})")
        if rdat:
            parts.append(f"R{rdat[0]:x}({len(rdat)})")
        if com:
            parts.append("C" + ",".join(f"{c:x}" for c in com[:3]) + ("+" if len(com) > 3 else ""))
        s = describe_strings(ev, i)
        if s:
            parts.append(s)
        callers = sorted(ev.callers.get(i, ()))
        near = [j - i for j in callers if abs(j - i) <= 40]
        parts.append(f"k{len(callers)}" + (f"{near}" if near else ""))
        fill = "" if f.fill == "nop" and f.address % 16 == 0 else f" [{padding_regime(f)}]"
        print(f"{i:4} {f.address:08x} {tag} {f.name[:38]:38} {f.area[:20]:20}{fill} "
              + " ".join(parts))


def cmd_build(ev: Evidence, write: bool):
    units = load_spec()
    unit_of = assign(units, ev)
    anchored, conflicts, fconf = analyse(units, ev, unit_of)
    problems = []
    for f in ev.funcs:
        for region, v, h in conflicts[f.index]:
            problems.append(f"{f.address:08x} {f.name}: owned {region} {v:08x} homed in "
                            f"{units[h].id}, assigned {units[unit_of[f.index]].id}")
    for u in units:
        files = {ev.file_anchor(i) for i in range(u.first, u.last + 1)} - {None}
        if len(files) > 1:
            problems.append(f"{u.id}: multiple __FILE__ anchors {sorted(files)}")
    for p in problems:
        print("conflict:", p, file=sys.stderr)
    conf = collections.Counter(u.boundary["confidence"] for u in units)
    fc = collections.Counter(fconf)
    named = sum(1 for u in units if u.name_basis != "none")
    print(f"units {len(units)} (named {named}); boundaries {dict(conf)}; "
          f"functions {dict(fc)}; conflicts {len(problems)}")
    if not write:
        return 1 if problems and "--strict" in sys.argv else 0

    with OUT_FUNCS.open("w", newline="", encoding="utf-8") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["address", "function_name", "unit_order", "unit_id", "unit_name",
                    "unit_kind", "confidence", "evidence", "current_source"])
        sources = source_markers()
        for f in ev.funcs:
            u = units[unit_of[f.index]]
            tokens = sorted(anchored[f.index])
            if not tokens:
                if u.anchors and u.anchors[0] < f.index < u.anchors[-1]:
                    tokens = ["bracketed"]
                else:
                    tokens = [f"boundary:{u.id}" if f.index <= (u.anchors[0] if u.anchors
                                                                 else u.last)
                              else f"boundary:{units[u.ordinal + 1].id}"
                              if u.ordinal + 1 < len(units) else "image-end"]
            if conflicts[f.index]:
                tokens.append("conflict")
            w.writerow([f"{f.address:08x}", f.name, u.ordinal, u.id, u.name, u.kind,
                        fconf[f.index], ";".join(tokens), sources.get(f.address, "")])

    with OUT_UNITS.open("w", newline="", encoding="utf-8") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["unit_order", "unit_id", "unit_name", "unit_kind", "name_basis",
                    "text_start", "text_end", "functions", "anchored_functions",
                    "boundary_confidence", "boundary_evidence", "boundary_slack",
                    "rdata_range", "data_range", "string_range", "bss_range", "note"])
        for k, u in enumerate(units):
            end = units[k + 1].start if k + 1 < len(units) else ev.binary.text[1]

            def rng(region):
                r = u.ranges.get(region)
                return f"{r[0]:08x}-{r[1]:08x}" if r else ""

            b = u.boundary
            slack = "" if b["slack"] is None else str(b["slack"])
            w.writerow([k, u.id, u.name, u.kind, u.name_basis, f"{u.start:08x}",
                        f"{end:08x}", u.last - u.first + 1, len(u.anchors),
                        b["confidence"], ";".join(b["kinds"] + b["steps"]) or "none",
                        slack, rng("rdata"), rng("data"), rng("string"), rng("bss"),
                        u.note])
    print(f"wrote {OUT_FUNCS.relative_to(ROOT)} and {OUT_UNITS.relative_to(ROOT)}")
    return 0


def cmd_place(ev: Evidence) -> int:
    """Move each boundary to the call-locality optimum inside its slack zone.

    Between the last anchored function of a unit and the first anchored
    function of the next, each unanchored function votes for the side its
    anchored callers/callees sit on.  The cut maximising agreeing votes wins;
    ties keep the curated position.  Rewrites the spec's start column.
    """
    units = load_spec()
    unit_of = assign(units, ev)
    anchored, _, _ = analyse(units, ev, unit_of)
    rows = [r for r in read_rows(SPEC) if not r["start"].startswith("#")]
    moved = 0
    for k in range(1, len(units)):
        left, right = units[k - 1], units[k]
        if not left.anchors or not right.anchors:
            continue
        if {"padding", "library", "segment", "file"} & set(right.boundary["kinds"]):
            continue
        a, b = left.anchors[-1], right.anchors[0]
        if b - a <= 1:
            continue
        votes = {}
        for i in range(a + 1, b):
            lv = rv = 0
            f = ev.funcs[i]
            neigh = set(ev.callers.get(i, ())) | {ev.by_addr[t].index for t in f.calls
                                                 if t in ev.by_addr}
            for j in neigh:
                if anchored[j] and unit_of[j] == k - 1:
                    lv += 1
                elif anchored[j] and unit_of[j] == k:
                    rv += 1
            votes[i] = (lv, rv)

        def score(cut):
            return sum(lv for i, (lv, rv) in votes.items() if i < cut) + \
                sum(rv for i, (lv, rv) in votes.items() if i >= cut)

        current = right.first
        best = max(range(a + 1, b + 1), key=lambda c: (score(c), c == current))
        if best != current and score(best) > score(current):
            rows[k]["start"] = f"{ev.funcs[best].address:08x}"
            moved += 1
            print(f"{right.id}: {current} -> {best} ({ev.funcs[best].name})")
    with SPEC.open("w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    print(f"moved {moved} boundaries")
    return 0


def cmd_layout() -> int:
    """Compare the written map with the current src/ layout."""
    rows = read_rows(OUT_FUNCS)
    units = collections.OrderedDict()
    files = collections.defaultdict(list)
    for r in rows:
        units.setdefault(r["unit_id"], []).append(r)
        if r["current_source"]:
            files[r["current_source"]].append(r)
    split = []
    for uid, members in units.items():
        srcs = collections.Counter(r["current_source"] or "(none)" for r in members)
        if len([s for s in srcs if s != "(none)"]) > 1:
            split.append((uid, members[0]["unit_name"], len(members), srcs))
    mixed = []
    for path, members in files.items():
        us = collections.Counter(r["unit_id"] for r in members)
        if len(us) > 1:
            mixed.append((path, len(members), us))
    # Order inversions inside each source file (by unit order then address).
    inversions = 0
    for path, members in files.items():
        seq = [int(r["unit_order"]) for r in members]
        inversions += sum(1 for a, b in zip(seq, seq[1:]) if b < a)
    print(f"units {len(units)}; units spread over >1 source file: {len(split)}; "
          f"source files mixing >1 unit: {len(mixed)} of {len(files)}")
    print("\nUnits split across source files (unit, name, functions, files):")
    for uid, name, n, srcs in split:
        top = ", ".join(f"{p.removeprefix('src/')}:{c}" for p, c in srcs.most_common(4))
        print(f"  {uid} {name or '-'} ({n}): {top}{' ...' if len(srcs) > 4 else ''}")
    print("\nSource files drawing from several units (file, functions, units):")
    for path, n, us in sorted(mixed, key=lambda t: -len(t[2])):
        top = ", ".join(f"{u}:{c}" for u, c in us.most_common(5))
        print(f"  {path} ({n}): {len(us)} units: {top}{' ...' if len(us) > 5 else ''}")
    return 0


def main(argv=None) -> int:
    global SPEC
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--exe", type=pathlib.Path, default=DEFAULT_EXE)
    ap.add_argument("--spec", type=pathlib.Path, default=SPEC,
                    help="curated unit list (default: %(default)s)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("evidence")
    p = sub.add_parser("propose")
    p.add_argument("--gamma", type=float, default=3.0)
    p = sub.add_parser("dossier")
    p.add_argument("lo")
    p.add_argument("hi")
    sub.add_parser("build")
    sub.add_parser("place")
    sub.add_parser("layout")
    p = sub.add_parser("check")
    p.add_argument("--strict", action="store_true")
    args = ap.parse_args(argv)

    SPEC = args.spec.resolve()
    binary = Binary(args.exe)
    ev = Evidence(binary, load_functions(binary))
    if args.cmd == "evidence":
        cmd_evidence(ev)
    elif args.cmd == "propose":
        cmd_propose(ev, args.gamma)
    elif args.cmd == "dossier":
        cmd_dossier(ev, int(args.lo, 16), int(args.hi, 16))
    elif args.cmd == "layout":
        return cmd_layout()
    elif args.cmd == "place":
        return cmd_place(ev)
    elif args.cmd == "build":
        return cmd_build(ev, True)
    elif args.cmd == "check":
        return cmd_build(ev, False)
    return 0


if __name__ == "__main__":
    sys.exit(main())
