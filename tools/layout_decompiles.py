#!/usr/bin/env python3
"""Layout a complete Ghidra pseudocode corpus from an exported REA evidence bundle.

Reads a REA `export_evidence_bundle` JSON file, collects every `batch_decompile`
item, deduplicates by procedure address, and writes:

  <out>/LEGORR.full.c      - one .c file with every function in address order
  <out>/manifest.csv       - address, symbol, pseudocode length per function
  <out>/README.md          - provenance and regeneration notes

Usage:
  python3 tools/decompile_layout.py <bundle.json> <out-dir>
"""

import csv
import json
import os
import re
import sys


def load_bundle(path):
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)


def collect_items(bundle):
    """Return {address: (name, pseudocode)} for every batch_decompile item."""
    funcs = {}
    for rec in bundle.get("records", []):
        if rec.get("operation") != "batch_decompile":
            continue
        nr = rec.get("normalized_result") or {}
        for item in nr.get("items", []):
            if item.get("status") != "ok":
                continue
            address = item.get("address")
            if not address:
                continue
            proc = item.get("procedure") or {}
            name = proc.get("name") or ""
            pseudo = item.get("pseudocode") or ""
            # Keep the first complete record for each address.
            funcs.setdefault(address, (name, pseudo))
    return funcs


def address_key(address):
    """Sort key for provider-normalized addresses like '0x401000'."""
    m = re.fullmatch(r"0x([0-9a-fA-F]+)", address)
    if m:
        return int(m.group(1), 16)
    return address  # EXTERNAL etc. sort last / lexically


def format_addr(address):
    """Render an address as 8-digit uppercase hex (repo convention)."""
    m = re.fullmatch(r"0x([0-9a-fA-F]+)", address)
    if m:
        return m.group(1).upper().rjust(8, "0")
    return address


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    bundle_path, out_dir = sys.argv[1], sys.argv[2]
    bundle = load_bundle(bundle_path)
    funcs = collect_items(bundle)

    ordered = sorted(funcs.items(), key=lambda kv: address_key(kv[0]))
    os.makedirs(out_dir, exist_ok=True)

    all_c_path = os.path.join(out_dir, "LEGORR.full.c")
    manifest_path = os.path.join(out_dir, "manifest.csv")
    readme_path = os.path.join(out_dir, "README.md")

    with open(all_c_path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(
            "/* LEGO Rock Raiders (LegoRR.exe) -- complete Ghidra pseudocode corpus.\n"
            " * Auto-generated from the REA evidence bundle; see README.md. */\n\n"
        )
        for address, (name, pseudo) in ordered:
            fh.write(f"// ==== {name or address} @ {format_addr(address)} ====\n\n")
            fh.write(pseudo)
            if not pseudo.endswith("\n"):
                fh.write("\n")
            fh.write("\n")

    with open(manifest_path, "w", encoding="utf-8", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(["address", "symbol", "pseudocode_bytes"])
        for address, (name, pseudo) in ordered:
            writer.writerow([format_addr(address), name or "", len(pseudo)])

    sha = bundle.get("artifacts")
    digest = ""
    if isinstance(sha, list) and sha:
        first = sha[0] or {}
        digest = (first.get("digest") or {}).get("sha256", "") or first.get("sha256", "")
    with open(readme_path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(
            "# Full decompile corpus\n\n"
            f"Complete Ghidra pseudocode for **LegoRR.exe** ({len(ordered)} local functions).\n\n"
            f"- `LEGORR.full.c` -- every function in address order, one block per function\n"
            f"- `manifest.csv` -- per-function index (address / symbol / size)\n"
            f"- SHA-256: `{digest or 'unknown'}`\n\n"
            "Regenerate with REA (`open_binary` -> `batch_decompile` -> "
            "`export_evidence_bundle`), then:\n\n"
            "    python3 tools/layout_decompiles.py <bundle.json> <out-dir>\n"
        )

    print(f"Wrote {len(ordered)} functions to {out_dir}")
    print(f"  {all_c_path} ({os.path.getsize(all_c_path)} bytes)")
    print(f"  {manifest_path} ({os.path.getsize(manifest_path)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())