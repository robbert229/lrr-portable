#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Check committed matching progress and render a PR comparison (stdlib only)."""
import argparse
import json
import hashlib
import re
from pathlib import Path
from binary_progress_hash import input_hash
from match_baseline import load_baseline, compare
import match_listings

PROGRESS = 'docs/coverage/binary-progress.json'
BASELINE = 'docs/coverage/match-baseline.txt'
LISTINGS = 'docs/coverage/match-listing-digests.txt'
MARKER = '<!-- matching-progress-report -->'


def read_progress(root):
    data = json.loads((root / PROGRESS).read_text(encoding='utf-8'))
    if data['schema_version'] != 2:
        raise ValueError('unsupported binary progress schema')
    return data


def check(base, head):
    head_hash = input_hash(head)
    data = read_progress(head)
    baseline = (head / BASELINE).read_bytes()
    listings = (head / LISTINGS).read_bytes()
    listing_fingerprints = re.findall(rb'^# input_sha256: ([0-9a-f]{64})$', listings, re.MULTILINE)
    fingerprints = re.findall(rb'^# input_sha256: ([0-9a-f]{64})$', baseline, re.MULTILINE)
    if (len(fingerprints) != 1 or fingerprints[0].decode() != head_hash or
            data.get('input_sha256') != head_hash or
            data.get('baseline_sha256') != hashlib.sha256(baseline).hexdigest() or
            listing_fingerprints != [head_hash.encode()] or
            data.get('listing_digests_sha256') != hashlib.sha256(listings).hexdigest()):
        raise ValueError('matching progress or baseline is stale or modified; run make match-progress and never hand-merge the generated files')
    print('Matching progress and baseline are fresh.')


def render(base, head):
    if (input_hash(base) == input_hash(head) and
            (base / PROGRESS).read_bytes() == (head / PROGRESS).read_bytes() and
            (base / BASELINE).read_bytes() == (head / BASELINE).read_bytes()):
        return MARKER + '\nNo matching-build impact.\n'
    old, new = read_progress(base), read_progress(head)
    metrics = [
        ('Text excess over chance (pp)', ('text_at_original_addresses', 'excess_percent_points'), 1),
        ('Text 16+ byte run excess (pp)', ('text_at_original_addresses', 'run_excess_percent_points'), 1),
        ('Identical functions at original address', ('fully_identical_functions_at_original_address', 'count'), 1),
        ('Exact-function code bytes', ('exact_function_code_bytes', 'bytes'), 1),
        ('Exact-function code coverage (%)', ('exact_function_code_bytes', 'percent'), 1),
        ('Adjacent inversions, full', ('function_order_inversions', 'full', 'adjacent'), -1),
        ('Adjacent inversions, game-only', ('function_order_inversions', 'game_only', 'adjacent'), -1),
        ('Accuracy, game-only (%)', ('accuracy_percent', 'game_only'), 1),
        ('Accuracy, full (%)', ('accuracy_percent', 'full'), 1),
    ]
    lines = [MARKER, '## Matching-build progress', '', '| Metric | Base | PR | Direction |', '| --- | ---: | ---: | --- |']
    for label, path, direction in metrics:
        a, b = old, new
        for key in path:
            a, b = a[key], b[key]
        delta = b - a
        arrow = '⬆️ improvement' if delta * direction > 0 else '⬇️ regression' if delta * direction < 0 else '—'
        lines.append(f'| {label} | {a} | {b} | {arrow} ({delta:+.2f}) |')
    lines += ['', '### Match baseline', '']
    old_scopes, new_scopes = load_baseline(base / BASELINE), load_baseline(head / BASELINE)
    evidence = None
    if (base / LISTINGS).is_file() and (head / LISTINGS).is_file():
        try:
            evidence = (match_listings.load(base / LISTINGS), match_listings.load(head / LISTINGS),
                        match_listings.source_index(root=base), match_listings.source_index(root=head))
        except (OSError, ValueError):
            pass
    for scope in ('game-only', 'full'):
        before, after = old_scopes[scope], new_scopes[scope]
        fell, rose, mapped, unmapped = compare(before, after)
        # Net on common functions avoids implying that missing scores are zero.
        net = sum(float(after[a][0]) - float(before[a][0]) for a in before.keys() & after.keys())
        lines.append(f'**{scope}:** {len(rose)} rose, {len(fell)} fell, {len(mapped)} newly mapped, {len(unmapped)} unmapped; net {net:+.2f} percentage points across common functions.')
        movers = sorted(rose + fell, key=lambda x: (-abs(float(x[3]) - float(x[2])), x[0]))[:5]
        if movers:
            lines.append('Top movers: ' + '; '.join(f'`0x{addr:08x}` {name.replace("|", "&#124;")} {old_score}% → {new_score}%' for addr, name, old_score, new_score in movers) + '.')
        for address, name, old_score, new_score in fell:
            detail = 'classification unavailable; instruction delta unknown'
            if evidence:
                old_lists, new_lists, old_source, new_source = evidence
                a, b = old_lists[scope].get(address), new_lists[scope].get(address)
                if a and b:
                    x, y = old_source.get(address), new_source.get(address)
                    changed = None if x is None or y is None else x != y
                    kind, delta = match_listings.classify(a, b, changed)
                    detail = f'{kind}; instructions {delta:+d}' + ('; owner review' if delta or kind in ('source-changed', 'source-unknown', 'drift-review') else '')
            lines.append(f'- `0x{address:08x}` {name.replace("|", "&#124;")}: {old_score}% → {new_score}% — {detail}')
        lines.append('')
    lines.append('Measurements are committed locally; CI checks input freshness, not the correctness of the measurement. Review drops under the entropy policy.')
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('check', 'render'))
    parser.add_argument('--base', type=Path, required=True)
    parser.add_argument('--head', type=Path, required=True)
    args = parser.parse_args()
    if args.command == 'check':
        check(args.base, args.head)
    else:
        print(render(args.base, args.head), end='')


if __name__ == '__main__':
    main()
