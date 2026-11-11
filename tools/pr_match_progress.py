#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Check committed matching progress and render a PR comparison (stdlib only)."""
import argparse
import json
from pathlib import Path
from binary_progress_hash import input_hash
from match_baseline import load_baseline, compare

PROGRESS = 'docs/coverage/binary-progress.json'
BASELINE = 'docs/coverage/match-baseline.txt'
MARKER = '<!-- matching-progress-report -->'


def read_progress(root):
    data = json.loads((root / PROGRESS).read_text(encoding='utf-8'))
    if data['schema_version'] != 2:
        raise ValueError('unsupported binary progress schema')
    return data


def check(base, head):
    base_hash, head_hash = input_hash(base), input_hash(head)
    # Existing stale main must not block a PR with no matching-build impact.
    if base_hash == head_hash and (base / PROGRESS).read_bytes() == (head / PROGRESS).read_bytes():
        print('No matching-build input or progress change.')
        return
    if read_progress(head)['input_sha256'] != head_hash:
        raise ValueError('binary progress is stale; run make match-progress and commit both outputs')
    print('Binary progress is fresh.')


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
    for scope in ('game-only', 'full'):
        before, after = old_scopes[scope], new_scopes[scope]
        fell, rose, mapped, unmapped = compare(before, after)
        # Net on common functions avoids implying that missing scores are zero.
        net = sum(float(after[a][0]) - float(before[a][0]) for a in before.keys() & after.keys())
        lines.append(f'**{scope}:** {len(rose)} rose, {len(fell)} fell, {len(mapped)} newly mapped, {len(unmapped)} unmapped; net {net:+.2f} percentage points across common functions.')
        movers = sorted(rose + fell, key=lambda x: (-abs(float(x[3]) - float(x[2])), x[0]))[:5]
        if movers:
            lines.append('Top movers: ' + '; '.join(f'`0x{addr:08x}` {name.replace("|", "&#124;")} {old_score}% → {new_score}%' for addr, name, old_score, new_score in movers) + '.')
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
