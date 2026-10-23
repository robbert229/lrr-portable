#!/usr/bin/env python3
"""Build a reconstruction-coverage ledger for the full LegoRR.exe corpus.

Classifies every local function in docs/decompiles/full/manifest.csv by
reconstruction status, using the repo's own address-anchoring conventions:

  - "ported"    : the function's entry address or FUN_* name is cited in
                  src/ (via `0x…`, `@ 0x…`, `entry 0x…`, or FUN_004xxxxx)
  - "named"     : has a human name in the full corpus heading (or a non-FUN
                  symbol in the manifest) but no src/ citation yet
  - "anonymous" : still FUN_* only, with no src/ citation

Outputs:
  docs/coverage/ledger.csv   - one row per corpus function
  docs/coverage/README.md    - summary + per-region breakdown
"""

import csv
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "docs/decompiles/full/manifest.csv"
FULL_CORPUS = ROOT / "docs/decompiles/full/LEGORR.full.c"
SUPPLEMENT = ROOT / "docs/decompiles/full/LEGORR.supplement.c"
SRC_DIR = ROOT / "src"
OUT_DIR = ROOT / "docs/coverage"


def norm(addr: str) -> str:
    """Normalize an address to 8-digit lowercase hex."""
    return re.sub(r"^0x", "", addr).lower().rjust(8, "0")


def scan_src_anchors() -> dict:
    """Map every corpus-entry address mentioned in src/ to citation counts."""
    anchors = {}
    for f in SRC_DIR.rglob("*"):
        if f.suffix not in (".cpp", ".hpp", ".c", ".h"):
            continue
        text = f.read_text(errors="replace")
        # 0x… hex tokens
        for m in re.finditer(r"(?:0x|0X)([0-9a-fA-F]{1,8})\b", text):
            anchors.setdefault(norm(m.group(1)), 0)
            anchors[norm(m.group(1))] += 1
        # FUN_004xxxxx names
        for m in re.finditer(r"FUN_([0-9a-fA-F]{6,8})\b", text):
            anchors.setdefault(norm(m.group(1)), 0)
            anchors[norm(m.group(1))] += 1
    return anchors


def read_manifest() -> list:
    rows = []
    with MANIFEST.open() as fh:
        reader = csv.DictReader(fh)
        for r in reader:
            r["address"] = norm(r["address"])
            rows.append(r)
    return rows


def read_corpus_names(path: pathlib.Path) -> dict:
    """Read human names from full-corpus section headings."""
    names = {}
    for line in path.read_text().splitlines():
        m = re.match(
            r"// ==== (.+?) @ ([0-9A-Fa-f]{6,8})"
            r"(?: \[Ghidra: .*?\])? ====$",
            line,
        )
        if m:
            address = norm(m.group(2))
            label = m.group(1)
            ghidra = re.search(r" \[Ghidra: (.*?)\]$", line)
            if ghidra:
                # An explicit Ghidra suffix means the leading label is the
                # human alias added by annotate_decompile_names.py.
                name = label
            elif re.fullmatch(r"FUN_[0-9A-Fa-f]{6,8}", label):
                # Raw export headings are not human names.
                continue
            else:
                name = label
            if address in names and names[address] != name:
                raise ValueError(f"conflicting full-corpus names at {address}")
            names[address] = name
    return names


def read_supplement_names(path: pathlib.Path) -> dict:
    """Read named entries preserved outside the Ghidra function manifest."""
    return read_corpus_names(path)


# Symbols that are compiler/runtime support or import thunks, not game logic.
# They will never be "ported" and must not count as unported game code.
RUNTIME_OR_IMPORT = re.compile(
    r"^(__|_|thunk_|entry|D3DRM\.DLL|DDRAW\.DLL|AVIFIL32\.DLL|DINPUT\.DLL|"
    r"DSOUND\.DLL|WINMM\.DLL|KERNEL32\.DLL|USER32\.DLL|GDI32\.DLL|"
    r"WINMM\.DLL|ole32\.DLL|WSOCK32\.DLL|mpr\.DLL|version\.DLL)"
)


def classify(rows, anchors, named):
    """rows: manifest rows; anchors: address->citation count from src/."""
    entries = [norm(r["address"]) for r in rows]
    out = []
    for r in rows:
        a = norm(r["address"])
        sym = r["symbol"]
        human = sym if not sym.startswith("FUN_") else None
        cited = anchors.get(a, 0)
        named_at = named.get(a)
        if cited > 0:
            status = "ported"
        elif RUNTIME_OR_IMPORT.match(sym or ""):
            status = "runtime-import"
        elif named_at or human:
            status = "named"
        else:
            # Interior citation? (a src/ address inside this function's body)
            interior = False
            for cited_addr, _ in anchors.items():
                if cited_addr == a:
                    continue
                if a <= cited_addr and (containing(entries, cited_addr) == a):
                    interior = True
                    break
            status = "anchored" if interior else "anonymous"
        out.append({
            "address": a,
            "symbol": sym,
            "pseudocode_bytes": int(r["pseudocode_bytes"]),
            "status": status,
            "corpus_name": named_at or human or "",
            "src_citations": cited,
        })
    return out


def containing(entries, addr):
    """Last corpus entry <= addr, or None."""
    prev = None
    for x in entries:
        if x <= addr:
            prev = x
        else:
            break
    return prev


def region(addr: str) -> str:
    """64 KB page label, matching the repo's 0x4000000-style page grouping."""
    return f"0x{int(addr, 16) >> 16:05x}xxxx"


def main():
    rows = read_manifest()
    anchors = scan_src_anchors()
    named = read_corpus_names(FULL_CORPUS)
    supplement = read_supplement_names(SUPPLEMENT)
    ledger = classify(rows, anchors, named)

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    with (OUT_DIR / "ledger.csv").open("w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["address", "symbol", "status", "corpus_name", "src_citations", "pseudocode_bytes"])
        for e in ledger:
            w.writerow([e["address"], e["symbol"], e["status"], e["corpus_name"],
                        e["src_citations"], e["pseudocode_bytes"]])

    total = len(ledger)
    from collections import Counter
    by_status = Counter(e["status"] for e in ledger)
    ported = [e for e in ledger if e["status"] == "ported"]
    named_e = [e for e in ledger if e["status"] == "named"]
    anon = [e for e in ledger if e["status"] == "anonymous"]
    runtime = [e for e in ledger if e["status"] == "runtime-import"]
    anchored = [e for e in ledger if e["status"] == "anchored"]

    # per-region breakdown of anonymous (biggest remaining work)
    by_region = Counter()
    for e in anon:
        by_region[region(e["address"])] += 1
    region_rows = sorted(by_region.items(), key=lambda kv: -kv[1])

    named_in_region = [e for e in ledger if e["status"] == "named"]

    md = [
        "# Reconstruction coverage ledger",
        "",
        f"Covers all **{total}** local functions in `docs/decompiles/full/manifest.csv`",
        "(the full Ghidra corpus, PR #126).",
        "",
        "Function-area assignments and responsibility boundaries are in",
        "[`function-areas.md`](function-areas.md) and [`function-areas.csv`](function-areas.csv).",
        "Revalidate the map after a corpus change with `python3 tools/function_areas.py`.",
        "",
        "## Status counts",
        "",
        f"| Status | Count | Share |",
        f"| --- | ---: | ---: |",
    ]
    for s in ("ported", "anchored", "named", "runtime-import", "anonymous"):
        md.append(f"| {s} | {by_status[s]} | {by_status[s]/total*100:.1f}% |")

    md += [
        "",
        "- **ported** — entry address or `FUN_*` name cited directly in `src/`",
        f"  ({len(ported)} functions)",
        "- **anchored** — only an *interior* address (a call site / offset inside",
        "  the body) is cited in `src/`; the function is touched but its entry is",
        f"  not named ({len(anchored)} functions)",
        "- **named** — human name in the full-corpus heading / manifest but no `src/`",
        f"  citation yet ({len(named_e)} functions)",
        "- **runtime-import** — CRT/runtime support or import thunks; not game",
        f"  logic, never ported ({len(runtime)} functions)",
        "- **anonymous** — `FUN_*` only, unreferenced in `src/`",
        f"  ({len(anon)} functions)",
        "",
        f"**Game logic remaining to port: {len(named_e) + len(anon) + len(anchored)} "
        f"({(len(named_e) + len(anon) + len(anchored)) / total * 100:.1f}%).**",
        "",
        "## Largest anonymous regions (by 64 KB page)",
        "",
        "| Region | Anonymous functions |",
        "| --- | ---: |",
    ]
    for r, c in region_rows:
        md.append(f"| `{r}` | {c} |")

    md += [
        "",
        "## Named-but-unported corpus functions",
        "",
        "Human-named in the corpus but with no `src/` citation yet.",
        "",
        "| Address | Name |",
        "| --- | --- |",
    ]
    for e in sorted(named_in_region, key=lambda e: e["address"]):
        md.append(f"| `0x{e['address']}` | {e['corpus_name']} |")

    md += [
        "",
        "## Suggested next port target",
        "",
        "The largest **named** cluster with no `src/` citation is the",
        "**savegame / generic-list / command subsystem** in `0x00416d60–0x00417fc0`:",
        "`Sav*` (save slot/tree/record), `List*` (named-object list), `Cmd*`",
        "(command/request matching), `Osf*`, `Task*`, `Checkpoint*`, `Reward*`.",
        "These already carry human names in the corpus, making them the cheapest",
        "high-value port. The per-function pseudocode for every one of them is in",
        "`docs/decompiles/full/LEGORR.full.c`.",
        "",
        "## Caveats",
        "",
        "- A `src/` citation of an entry address is a *strong* anchor (the file",
        "  is anchored to that function) but is not proof the port is complete.",
        f"- {len(supplement)} named entries preserved in `LEGORR.supplement.c` are",
        "  absent from the full-corpus manifest at those exact entry addresses.",
        "  They are retained for lookup but excluded from the 2,564-function",
        "  coverage counts.",
        "",
        "Regenerate: `python3 tools/coverage_ledger.py`",
        "",
    ]
    (OUT_DIR / "README.md").write_text("\n".join(md))

    print(f"total={total} ported={by_status['ported']} "
          f"named={by_status['named']} anonymous={by_status['anonymous']}")
    print(f"wrote {OUT_DIR/'ledger.csv'} and {OUT_DIR/'README.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
