#!/usr/bin/env python3
"""rrae.py — ground truth for the `.ae` entity assembly (src/gods98/entity).

Extracts the per-entity activity table the exe assembles in FUN_00472cd0's
ACT branch (finding 12 §4) from the shipped WADs, reproducing the block
parser (FUN_00478e60), the root-path lookup (FUN_004792e0, `*`-prefix at the
first token), and the activity walk (kids of `<root>=LegoRR Activities`,
`!`-key reduce-animation gate on feature bit 0x400000, FILE-gated rows,
TRANSCOEF/TRIGGER/LWSFILE/LOOPING/SAMPLE typed reads, root SCALE).

Emits the same counts + FNV-1a digest test_entity.cpp folds, so the corpus
constants (`REAL_*` in test_entity.cpp) are reproducible:

    python3 tools/rrae.py truth source/LegoRR0.wad source/LegoRR1.wad
    python3 tools/rrae.py truth source/LegoRR0.wad source/LegoRR1.wad --reduce-animation
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import rrwad  # noqa: E402

ROOT = ["LegoRR"]


def block_pass1(data):
    buf = bytearray(data)
    comment = False
    for i in range(len(buf)):
        c = buf[i]
        if c == ord(';'):
            comment = True
        elif c == ord('\n'):
            comment = False
        if comment or c in (0x20, 0x09, 0x0d, 0x0a):
            buf[i] = 0
    return buf


def tokenize(buf):
    toks = []
    i = 0
    n = len(buf)
    while i < n:
        while i < n and buf[i] == 0:
            i += 1
        if i >= n:
            break
        j = i
        while j < n and buf[j] != 0:
            j += 1
        toks.append(bytes(buf[i:j]).decode("latin1"))
        i = j
    return toks


def parse_records(text):
    buf = block_pass1(text)
    recs = []
    depth = 0
    stack = [None]
    pending = None
    for t in tokenize(buf):
        if t == '}':
            if pending is not None:
                recs.append({"depth": pending[0], "parent": pending[1],
                             "name": pending[2], "value": None})
                pending = None
            if depth > 0:
                depth -= 1
                stack.pop()
            continue
        if pending is None:
            pending = (depth, stack[-1], t)
        else:
            d, par, name = pending
            idx = len(recs)
            recs.append({"depth": d, "parent": par, "name": name,
                         "value": t if t != '{' else '{'})
            if t == '{':
                depth += 1
                stack.append(idx)
            pending = None
    if pending is not None:
        recs.append({"depth": pending[0], "parent": pending[1],
                     "name": pending[2], "value": None})
    return recs


def ci_ok(a, b):
    return (a or "").lower() == (b or "").lower()


def find_path(recs, segs):
    n = len(segs)
    for r in recs:
        if r["depth"] != n - 1:
            continue
        if not ci_ok(r["name"], segs[-1]):
            continue
        ok = True
        cur = r["parent"]
        for k in range(n - 1, 0, -1):
            seg = segs[k - 1]
            if cur is None:
                ok = False
                break
            rec = recs[cur]
            rn = rec["name"] or ""
            if k == 1 and '*' in rn:
                prefix = rn.split('*', 1)[0]
                ok = seg.lower().startswith(prefix.lower())
            else:
                ok = rn.lower() == seg.lower()
            if not ok:
                break
            cur = rec["parent"]
        if ok:
            return r
    return None


def value(recs, segs):
    r = find_path(recs, segs)
    if r is None or r["value"] is None:
        return None
    return r["value"]


def atoi(s):
    if s is None:
        return 0
    s = s.lstrip()
    if not s:
        return 0
    neg = s[0] == '-'
    if s[0] in '+-':
        s = s[1:]
    v = 0
    for c in s:
        if '0' <= c <= '9':
            v = v * 10 + (ord(c) - 48)
        else:
            break
    return -v if neg else v


def atof(s):
    if s is None:
        return 0.0
    s = s.lstrip()
    if not s:
        return 0.0
    try:
        return float(s)
    except ValueError:
        return 0.0


def parse_bool(s):
    if s is None:
        return 2
    low = s.lower()
    if low in ("yes", "true", "on"):
        return 1
    if low in ("no", "false", "off"):
        return 0
    return 2


def entity_assembly(recs, feature=0):
    act = find_path(recs, ROOT + ["Activities"])
    if act is None:
        return None
    pidx = recs.index(act)
    rows = []
    for i, r in enumerate(recs):
        if r["parent"] != pidx:
            continue
        key = r["name"]
        grp = r["value"]
        if key is None:
            continue
        if key.startswith('!'):
            if feature & 0x400000:
                continue
            key = key[1:]
        if grp is None:
            continue
        filev = value(recs, ROOT + [grp, "FILE"])
        if not filev:
            continue
        trans = atof(value(recs, ROOT + [grp, "TRANSCOEF"]))
        trig = atoi(value(recs, ROOT + [grp, "TRIGGER"]))
        lws = parse_bool(value(recs, ROOT + [grp, "LWSFILE"]))
        loop = parse_bool(value(recs, ROOT + [grp, "LOOPING"]))
        samp = value(recs, ROOT + [grp, "SAMPLE"]) or ""
        rows.append({"key": key, "name": grp, "file": filev,
                     "transcoef": trans, "trigger": trig,
                     "lwsfile": lws, "looping": loop, "sample": samp})
    scale = atof(value(recs, ROOT + ["SCALE"]))
    return (scale, rows)


def fnv1a(h, s):
    if isinstance(s, str):
        s = s.encode("latin1")
    for b in s:
        h = ((h ^ b) * 16777619) & 0xffffffff
    return h


def qtrans(v):
    return int(v * 10000 + (0.5 if v >= 0 else -0.5))


def fold_row(h, r):
    parts = [r["key"], r["name"], r["file"], r["sample"],
             str(r["lwsfile"]), str(r["looping"]), str(r["trigger"]),
             str(qtrans(r["transcoef"]))]
    for p in parts:
        h = fnv1a(h, p)
        h = fnv1a(h, "\xff")
    return h


def run_truth(wad_paths, feature):
    wads = [rrwad.Wad(p) for p in wad_paths]
    entries = []
    seen = set()
    for w in wads:
        for nm in w.names:
            key = nm.lower().replace("\\", "/")
            if key in seen:
                continue
            seen.add(key)
            entries.append((key, w))
    entries.sort(key=lambda x: x[0])

    files = 0
    tbl = []
    file_scales = []
    activity_count = 0
    with_file = 0
    sample_count = 0
    lws_true = 0
    loop_nz = 0
    trig_sum = 0
    tc_nz = 0
    sc_nz = 0

    for key, w in entries:
        if not key.endswith(".ae"):
            continue
        raw = key.replace("/", "\\")
        idx = w.index_for(raw)
        recs = parse_records(w.payload(idx))
        asm = entity_assembly(recs, feature)
        files += 1
        if asm is None:
            tbl.append(None)
            file_scales.append(0)
            continue
        scale, rows = asm
        file_scales.append(qtrans(scale) if scale != 0.0 else 0)
        tbl.append(rows)
        activity_count += len(rows)
        with_file += sum(1 for r in rows if r["file"])
        sample_count += sum(1 for r in rows if r["sample"])
        lws_true += sum(1 for r in rows if r["lwsfile"] == 1)
        loop_nz += sum(1 for r in rows if r["looping"] != 0)
        trig_sum += sum(r["trigger"] for r in rows)
        tc_nz += sum(1 for r in rows if r["transcoef"] != 0.0)
        sc_nz += 1 if scale != 0.0 else 0

    h = 2166136261
    for rows, qs in zip(tbl, file_scales):
        h = fnv1a(h, str(qs))
        if rows is None:
            continue
        for r in rows:
            h = fold_row(h, r)

    return dict(ae=files, activities=activity_count, with_file=with_file,
                sample=sample_count, lws=lws_true, loop=loop_nz,
                trigger=trig_sum, transcoef_nz=tc_nz, scale_nz=sc_nz,
                digest=h)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("truth", help="emit the corpus aggregate + digest")
    p.add_argument("wads", nargs="+", help="LegoRR*.wad paths (mount order)")
    p.add_argument("--reduce-animation", action="store_true",
                   help="fold with feature bit 0x400000 ('!' rows dropped)")
    args = ap.parse_args()
    m = run_truth(args.wads, 0x400000 if args.reduce_animation else 0)
    tag = " reduce-animation" if args.reduce_animation else ""
    print(f"ae           {m['ae']}")
    print(f"activities   {m['activities']} rows")
    print(f"with_file    {m['with_file']}")
    print(f"sample       {m['sample']}")
    print(f"lws_true     {m['lws']}")
    print(f"loop_nz      {m['loop']}")
    print(f"trigger_sum  {m['trigger']}")
    print(f"transcoef_nz {m['transcoef_nz']}")
    print(f"scale_nz     {m['scale_nz']}")
    print(f"digest       0x{m['digest']:08x}{tag}")


if __name__ == "__main__":
    main()
