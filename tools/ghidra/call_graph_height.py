#!/usr/bin/env python3
"""Build a Ghidra-corpus call graph and rank functions by graph height.

The full Ghidra decompile corpus is split into function blocks with address
headings. Resolved calls to functions in the corpus become directed caller to
callee edges. Recursive strongly connected components are condensed before
height is calculated.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HEADER = re.compile(
    r"^// ==== (.*?) @ ([0-9A-Fa-f]{8})(?: \[Ghidra: (.*?)\])? ====$",
    re.MULTILINE,
)
CALL = re.compile(r"(?<![A-Za-z0-9_$])([A-Za-z_$][A-Za-z0-9_$]*)\s*\(")


def parse_corpus(path: Path, manifest_path: Path):
    text = path.read_text(encoding="utf-8", errors="replace")
    manifest = list(csv.DictReader(manifest_path.open(newline="", encoding="utf-8")))
    symbol_to_address = {}
    for row in manifest:
        address = int(row["address"], 16)
        symbol = row["symbol"].strip()
        if symbol:
            symbol_to_address[symbol] = address
        symbol_to_address[f"FUN_{address:08x}"] = address
        symbol_to_address[f"FUN_{address:08X}"] = address

    headers = list(HEADER.finditer(text))
    nodes = {}
    for index, header in enumerate(headers):
        start = header.end()
        end = headers[index + 1].start() if index + 1 < len(headers) else len(text)
        address = int(header.group(2), 16)
        name = header.group(1)
        ghidra_symbol = header.group(3) or name
        symbol_to_address[name] = address
        symbol_to_address[ghidra_symbol] = address
        nodes[address] = {"name": name, "body": text[start:end]}

    if len(nodes) != len(manifest):
        raise ValueError(
            f"Corpus has {len(nodes)} function headings but manifest has {len(manifest)} entries"
        )

    edges = set()
    for caller, node in nodes.items():
        body = node["body"]
        # Ignore the declaration/signature so it cannot look like recursion.
        opening_brace = body.find("{")
        if opening_brace < 0:
            continue
        code = mask_comments_and_literals(body[opening_brace + 1 :])
        for match in CALL.finditer(code):
            callee = symbol_to_address.get(match.group(1))
            if callee is not None and callee in nodes:
                edges.add((caller, callee))
    return nodes, edges


def mask_comments_and_literals(source: str) -> str:
    """Mask C comments and strings while preserving offsets and newlines."""
    chars = list(source)
    i = 0
    state = "code"
    quote = ""
    while i < len(source):
        ch = source[i]
        nxt = source[i + 1] if i + 1 < len(source) else ""
        if state == "code":
            if ch == "/" and nxt == "/":
                chars[i] = chars[i + 1] = " "
                i += 2
                state = "line"
                continue
            if ch == "/" and nxt == "*":
                chars[i] = chars[i + 1] = " "
                i += 2
                state = "block"
                continue
            if ch in "'\"":
                quote = ch
                chars[i] = " "
                state = "literal"
        elif state == "line":
            if ch == "\n":
                state = "code"
            else:
                chars[i] = " "
        elif state == "block":
            if ch == "*" and nxt == "/":
                chars[i] = chars[i + 1] = " "
                i += 2
                state = "code"
                continue
            if ch != "\n":
                chars[i] = " "
        else:
            if ch == "\\":
                chars[i] = " "
                if i + 1 < len(source) and source[i + 1] != "\n":
                    chars[i + 1] = " "
                i += 2
                continue
            if ch == quote:
                chars[i] = " "
                state = "code"
            elif ch != "\n":
                chars[i] = " "
        i += 1
    return "".join(chars)


def strongly_connected_components(nodes, edges):
    adjacency = {address: [] for address in nodes}
    for caller, callee in edges:
        adjacency[caller].append(callee)

    sys.setrecursionlimit(max(sys.getrecursionlimit(), len(nodes) * 2 + 100))
    index = 0
    indices = {}
    lowlinks = {}
    stack = []
    on_stack = set()
    components = []

    def visit(node):
        nonlocal index
        indices[node] = lowlinks[node] = index
        index += 1
        stack.append(node)
        on_stack.add(node)
        for neighbor in adjacency[node]:
            if neighbor not in indices:
                visit(neighbor)
                lowlinks[node] = min(lowlinks[node], lowlinks[neighbor])
            elif neighbor in on_stack:
                lowlinks[node] = min(lowlinks[node], indices[neighbor])
        if lowlinks[node] == indices[node]:
            component = []
            while True:
                member = stack.pop()
                on_stack.remove(member)
                component.append(member)
                if member == node:
                    break
            components.append(component)

    for node in nodes:
        if node not in indices:
            visit(node)
    return adjacency, components


def calculate_heights(nodes, edges):
    adjacency, components = strongly_connected_components(nodes, edges)
    component_of = {
        node: component_id
        for component_id, component in enumerate(components)
        for node in component
    }
    dag = [set() for _ in components]
    for caller, callee in edges:
        source = component_of[caller]
        target = component_of[callee]
        if source != target:
            dag[source].add(target)

    heights = {}

    def height(component_id):
        if component_id not in heights:
            children = dag[component_id]
            heights[component_id] = 0 if not children else 1 + max(height(c) for c in children)
        return heights[component_id]

    for component_id in range(len(components)):
        height(component_id)

    recursive = set()
    for component_id, component in enumerate(components):
        if len(component) > 1 or any((node, node) in edges for node in component):
            recursive.add(component_id)

    in_degree = defaultdict(int)
    out_degree = defaultdict(int)
    for caller, callee in edges:
        out_degree[caller] += 1
        in_degree[callee] += 1

    records = []
    for address, node in nodes.items():
        component_id = component_of[address]
        records.append({
            "address": f"0x{address:08x}",
            "name": node["name"],
            "height": heights[component_id],
            "scc_size": len(components[component_id]),
            "recursive_component": component_id in recursive,
            "in_degree": in_degree[address],
            "out_degree": out_degree[address],
        })
    records.sort(key=lambda row: (-row["height"], int(row["address"], 16)))
    for rank, row in enumerate(records, 1):
        row["rank"] = rank
    return records


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus", type=Path, default=ROOT / "docs/decompiles/full/LEGORR.full.c")
    parser.add_argument("--manifest", type=Path, default=ROOT / "docs/decompiles/full/manifest.csv")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "docs/coverage/call-graph")
    args = parser.parse_args()

    nodes, edges = parse_corpus(args.corpus, args.manifest)
    ranking = calculate_heights(nodes, edges)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    ranking_path = args.output_dir / "functions-by-height.csv"
    edge_path = args.output_dir / "edges.csv"

    with ranking_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=[
            "rank", "address", "name", "height", "scc_size",
            "recursive_component", "in_degree", "out_degree",
        ])
        writer.writeheader()
        writer.writerows(ranking)
    with edge_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.writer(output)
        writer.writerow(["caller", "callee"])
        for caller, callee in sorted(edges):
            writer.writerow([f"0x{caller:08x}", f"0x{callee:08x}"])

    print(f"functions={len(nodes)} edges={len(edges)}")
    print(f"highest_height={ranking[0]['height']} at {ranking[0]['address']} ({ranking[0]['name']})")
    print(f"wrote {ranking_path} and {edge_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
