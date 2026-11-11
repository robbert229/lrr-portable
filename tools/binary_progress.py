#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Compare the original PE with the matching build."""
import hashlib
import argparse
import json
import statistics
from pathlib import Path

from binary_progress_hash import input_hash
from translation_units import Binary, load_functions, ROOT


def percent(n, d):
    return round(100 * n / d, 2) if d else 0.0


RUN_LENGTH = 16
SCHEMA_VERSION = 2


def run_bytes(original, rebuilt):
    total = run = 0
    for a, b in zip(original, rebuilt):
        if a == b:
            run += 1
        else:
            if run >= RUN_LENGTH:
                total += run
            run = 0
    return total + (run if run >= RUN_LENGTH else 0)


def compare_bytes(original, rebuilt):
    same = sum(a == b for a, b in zip(original, rebuilt))
    # Compare against deterministic circular shifts of the original. This
    # preserves the full denominator even for short resource directories.
    shifts = sorted({max(1, len(original) * n // 11) for n in (1, 2, 3, 4, 5)}) if len(original) > 1 else []
    chance = [sum(a == b for a, b in zip(original, original[k:] + original[:k])) for k in shifts]
    run_chance = [run_bytes(original, original[k:] + original[:k]) for k in shifts]
    baseline = statistics.median(chance) if chance else 0
    run_baseline = statistics.median(run_chance) if run_chance else 0
    runs = run_bytes(original, rebuilt)
    return {'original_bytes': len(original), 'rebuilt_bytes': len(rebuilt),
            'identical_bytes': same, 'identical_percent': percent(same, len(original)),
            'chance_percent': percent(baseline, len(original)),
            'excess_percent_points': round(100 * (same - baseline) / len(original), 2) if original else 0.0,
            'run_identical_bytes': runs, 'run_identical_percent': percent(runs, len(original)),
            'run_chance_percent': percent(run_baseline, len(original)),
            'run_excess_percent_points': round(100 * (runs - run_baseline) / len(original), 2) if original else 0.0}


def inversions(entries):
    # Count pairs, not displaced functions. Equal rebuilt addresses are ties.
    values = [int(e['recomp'], 16) for e in sorted(entries, key=lambda e: int(e['address'], 16)) if e.get('recomp')]
    def count(items):
        if len(items) < 2:
            return items, 0
        mid = len(items) // 2
        left, a = count(items[:mid]); right, b = count(items[mid:])
        merged = []; i = j = 0
        while i < len(left) and j < len(right):
            if left[i] <= right[j]:
                merged.append(left[i]); i += 1
            else:
                merged.append(right[j]); j += 1; a += len(left) - i
        return merged + left[i:] + right[j:], a + b
    return {"adjacent": sum(a > b for a, b in zip(values, values[1:])), "pairs": count(values)[1]}


def section_bytes(pe, name):
    for section in pe.sections:
        if section.Name.rstrip(b'\0').decode('ascii') == name:
            return section.get_data()[:section.Misc_VirtualSize]
    return b''


def directory_bytes(pe, index):
    d = pe.OPTIONAL_HEADER.DATA_DIRECTORY[index]
    return pe.get_data(d.VirtualAddress, d.Size) if d.VirtualAddress and d.Size else b''


def report(orig_path, rebuilt_path, report_dir):
    import pefile
    original = Binary(orig_path)
    rebuilt = pefile.PE(str(rebuilt_path))
    orig = original.pe
    sections = {name: compare_bytes(section_bytes(orig, name), section_bytes(rebuilt, name))
                for name in ('.text', '.rdata', '.data')}
    for name, index in (('imports', 1), ('resources', 2)):
        sections[name] = compare_bytes(directory_bytes(orig, index), directory_bytes(rebuilt, index))
    text = section_bytes(orig, '.text')
    rva = original.text[0] - rebuilt.OPTIONAL_HEADER.ImageBase
    mapped = rebuilt.get_memory_mapped_image()
    at_address = compare_bytes(text, mapped[rva:rva + len(text)] if rva >= 0 else b'')
    full = json.loads((report_dir / 'results.json').read_text())['data']
    game = json.loads((report_dir / 'game-only.json').read_text())['data']
    exact = {int(e['address'], 16) for e in full if e.get('matching') == 1.0}
    funcs = load_functions(original)
    covered = sum(f.end - f.address for f in funcs if f.address in exact)
    positioned = [f for f in funcs if f.address in exact and any(
        int(e['address'], 16) == f.address and e.get('recomp') and
        int(e['recomp'], 16) == f.address for e in full) and
        original.read(f.address, f.end - f.address) ==
        mapped[f.address - rebuilt.OPTIONAL_HEADER.ImageBase:f.end - rebuilt.OPTIONAL_HEADER.ImageBase]]
    def mean(entries):
        return round(sum(round(float(e.get('matching', 0)) * 10000) / 100 for e in entries) / len(entries), 2) if entries else 0.0
    fields = [('linker_major', 'MajorLinkerVersion'), ('linker_minor', 'MinorLinkerVersion'),
              ('section_alignment', 'SectionAlignment'), ('entry_point_rva', 'AddressOfEntryPoint'),
              ('image_size', 'SizeOfImage'), ('subsystem', 'Subsystem')]
    headers = {name: {'original': getattr(orig.OPTIONAL_HEADER, field), 'rebuilt': getattr(rebuilt.OPTIONAL_HEADER, field)}
               for name, field in fields if getattr(orig.OPTIONAL_HEADER, field) != getattr(rebuilt.OPTIONAL_HEADER, field)}
    return {'schema_version': SCHEMA_VERSION, 'minimum_identical_run_bytes': RUN_LENGTH, 'input_sha256': input_hash(), 'sections': sections,
            'text_at_original_addresses': at_address, 'function_order_inversions': {'full': inversions(full), 'game_only': inversions(game)},
            'fully_identical_functions_at_original_address': {'count': len(positioned), 'bytes': sum(f.end - f.address for f in positioned)},
            'exact_function_code_bytes': {'bytes': covered, 'original_text_bytes': len(text), 'percent': percent(covered, len(text))},
            'accuracy_percent': {'game_only': mean(game), 'full': mean(full)}, 'pe_header_differences': headers}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, default=ROOT / 'reference/LegoRR.exe')
    parser.add_argument('--rebuilt', type=Path, default=ROOT / 'build-reccmp/LegoRR.exe')
    parser.add_argument('--report-dir', type=Path, default=ROOT / 'build-reccmp/reccmp-report')
    parser.add_argument('--output', type=Path, default=ROOT / 'docs/coverage/binary-progress.json')
    args = parser.parse_args()
    result = report(args.original, args.rebuilt, args.report_dir)
    result['baseline_sha256'] = hashlib.sha256((ROOT / 'docs/coverage/match-baseline.txt').read_bytes()).hexdigest()
    result['listing_digests_sha256'] = hashlib.sha256((ROOT / 'docs/coverage/match-listing-digests.txt').read_bytes()).hexdigest()
    output = json.dumps(result, indent=2, sort_keys=True) + '\n'
    args.output.write_text(output, encoding='utf-8')
    print(output, end='')

if __name__ == '__main__':
    main()
