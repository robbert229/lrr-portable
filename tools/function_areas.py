#!/usr/bin/env python3
"""Validate and summarize the hand-reviewed function-area assignments."""

import csv
import pathlib
import sys
from collections import Counter, defaultdict


ROOT = pathlib.Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "docs/decompiles/full/manifest.csv"
LEDGER = ROOT / "docs/coverage/ledger.csv"
AREAS = ROOT / "docs/coverage/function-areas.csv"
README = ROOT / "docs/coverage/function-areas.md"

DOMAIN_DESCRIPTIONS = {
    "gameplay-simulation": "Entity behavior, commands, objectives, tasks, combat, simulation state, and progression.",
    "world-and-terrain": "Terrain cells and grids, spatial queries, placement searches, and route transitions.",
    "player-interface": "Front end, input interpretation, menus, HUD, advisor/help, and display-facing controls.",
    "scene-and-rendering": "Scene objects and resources, models, viewports, render surfaces, and visual effects.",
    "audio-and-video": "Sound, spatial playback, streams, and video playback or decoding.",
    "game-data-and-assets": "Configuration, map and level loading, archives, registries, and presentation/resource records.",
    "platform-and-runtime": "Win32/DirectX interfaces, startup/window services, CRT, and operating-system/runtime support.",
    "shared-primitives": "Cross-cutting state accessors, lists/strings, pools, math, and small reusable helpers.",
}


def read_index(path: pathlib.Path, key: str) -> dict:
    result = {}
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            address = row[key].lower().removeprefix("0x").zfill(8)
            if address in result:
                raise ValueError(f"duplicate address {address} in {path}")
            result[address] = row
    return result


def build_readme() -> str:
    manifest = read_index(MANIFEST, "address")
    ledger = read_index(LEDGER, "address")
    rows = read_index(AREAS, "address")

    if set(manifest) != set(ledger):
        raise ValueError("coverage ledger addresses do not match the full manifest")
    if set(manifest) != set(rows):
        missing = sorted(set(manifest) - set(rows))
        extra = sorted(set(rows) - set(manifest))
        raise ValueError(
            f"area map/manifest mismatch: {len(missing)} missing, {len(extra)} extra; "
            f"first missing={missing[:3]}, first extra={extra[:3]}"
        )

    areas = defaultdict(list)
    for address, row in rows.items():
        if row["domain"] not in DOMAIN_DESCRIPTIONS:
            raise ValueError(f"unknown domain {row['domain']!r} at {address}")
        if not all(row.get(field) for field in (
            "function_name", "area_id", "area_name", "area_boundary",
            "classification_basis", "confidence",
        )):
            raise ValueError(f"incomplete area assignment at {address}")
        expected = ledger[address].get("corpus_name") or manifest[address]["symbol"]
        if row["function_name"] != expected:
            raise ValueError(
                f"function name mismatch at {address}: "
                f"{row['function_name']!r} != {expected!r}"
            )
        if row["classification_basis"] not in {
            "body", "name+body", "name", "import-symbol", "address-cluster", "unresolved"
        }:
            raise ValueError(f"unknown classification basis at {address}")
        if row["confidence"] not in {"high", "medium", "low"}:
            raise ValueError(f"unknown confidence value at {address}")
        areas[row["area_id"]].append(row)

    domain_counts = Counter(row["domain"] for row in rows.values())
    confidence = Counter(row["confidence"] for row in rows.values())
    basis = Counter(row["classification_basis"] for row in rows.values())
    lines = [
        "# Function areas",
        "",
        f"This first-pass map assigns every one of the **{len(rows)}** functions in the full manifest to one of **{len(areas)}** bounded responsibility areas, organized under {len(DOMAIN_DESCRIPTIONS)} broader domains.",
        "It classifies where each function belongs; it does not claim every function is fully understood or ported. Target: shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.",
        "",
        "## How to read the map",
        "",
        "- [`function-areas.csv`](function-areas.csv) contains one row per manifest address, with its domain, area, basis, confidence, and any function-specific exception note.",
        "- Area boundaries describe what responsibility is included and where it stops. A function can sit near a boundary; `classification_note` preserves a narrower exception when needed.",
        "- **Basis**: `body` means the pseudocode body supports the assignment; `name+body` uses both; `name` uses the human alias; `import-symbol` uses an import/runtime symbol; `address-cluster` relies on a surrounding cluster; `unresolved` means the role remains opaque and the assignment is provisional.",
        "- **Confidence** describes confidence in area placement, not completeness of behavioral understanding: `high` = the area is clear; `medium` = plausible placement with some ambiguity; `low` = provisional placement that needs follow-up.",
        "- Port/reconstruction status remains in [`ledger.csv`](ledger.csv). Address partitions used during review are not semantic boundaries.",
        "",
        f"Assignment confidence: {confidence['high']} high, {confidence['medium']} medium, {confidence['low']} low. Basis counts: {', '.join(f'{name} {basis[name]}' for name in sorted(basis))}.",
        f"{basis['unresolved']} functions with opaque pseudocode are provisionally grouped by surrounding subsystem. Three CRT no-op stubs have their own `runtime-support-unresolved` area.",
        "",
        "## Broader domains",
        "",
        "| Domain | Scope | Functions | Areas |",
        "| --- | --- | ---: | ---: |",
    ]
    for domain, description in DOMAIN_DESCRIPTIONS.items():
        count = sum(1 for area in areas.values() if area[0]["domain"] == domain)
        lines.append(f"| `{domain}` | {description} | {domain_counts[domain]} | {count} |")

    lines += ["", "## Bounded areas", ""]
    for domain, description in DOMAIN_DESCRIPTIONS.items():
        area_ids = sorted(
            area_id for area_id, members in areas.items()
            if members[0]["domain"] == domain
        )
        if not area_ids:
            continue
        lines += [
            f"### {domain}",
            "",
            "| Area ID | Functions | Responsibility boundary |",
            "| --- | ---: | --- |",
        ]
        for area_id in area_ids:
            members = areas[area_id]
            definitions = {
                (row["area_name"], row["area_boundary"], row["domain"])
                for row in members
            }
            if len(definitions) != 1:
                raise ValueError(f"inconsistent area definition for {area_id}")
            name, boundary, _ = next(iter(definitions))
            lines.append(f"| `{area_id}` — {name} | {len(members)} | {boundary} |")
        lines.append("")
    return "\n".join(lines)


def main() -> int:
    text = build_readme()
    README.write_text(text, encoding="utf-8")
    print(f"validated and wrote {README}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, KeyError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(1)
