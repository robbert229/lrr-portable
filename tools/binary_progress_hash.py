#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Hash tracked matching-build and measurement inputs using the standard library."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess

import link_order

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = 'tools/binary_progress_inputs.txt'
FIXED = ('CMakeLists.txt', 'cross/msvc5-toolchain.cmake', 'tools/reccmp.sh',
         'tools/clean_reccmp_report_names.py', 'tools/reccmp_full_diff.py', 'tools/match_listings.py',
         'tools/match_baseline.py', 'tools/binary_progress.py',
         'tools/binary_progress_hash.py', 'tools/translation_units.py',
         'tools/link_order.py', link_order.LIST,
         'tools/strip_pe_debug.py', 'docs/decompiles/full/manifest.csv',
         'docs/coverage/ledger.csv', 'docs/coverage/function-areas.csv', MANIFEST)


def selected_paths(root, tracked):
    """CMake and the link-order list declare translation units; conservatively
    include project headers.

    Inactive native and test branches are excluded. Headers are retained even
    when not currently included, since conditional includes are compiler-specific.
    """
    cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    prefixes = {'SRC_ROOT': 'src', 'ENGINE_DIR': 'src/gods98',
                'GAME_DIR': 'src/game', 'CRT_DIR': 'src/crt',
                'PLATFORM_DIR': 'src/platform'}
    sources = {prefixes[var] + '/' + name for var, name in re.findall(
        r'\$\{(SRC_ROOT|ENGINE_DIR|GAME_DIR|CRT_DIR|PLATFORM_DIR)\}/([\w./-]+\.(?:cpp|c|rc))', cmake)}
    if (root / link_order.LIST).is_file():
        sources.update(link_order.sources(link_order.load(root), platform=None))
    def selected(name):
        if name in FIXED:
            return True
        if not name.startswith('src/') or name.startswith('src/platform/native/'):
            return False
        if Path(name).name.startswith('test_'):
            return False
        return (name in sources or name.startswith('src/compat/msvc5/') or
                Path(name).suffix.lower() in ('.h', '.hpp', '.inl'))
    return sorted(name for name in tracked if selected(name))


def input_paths(root):
    try:
        result = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'],
                                check=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    except (OSError, subprocess.CalledProcessError):
        # Source archives carry an explicit snapshot of the tracked input list.
        return (root / MANIFEST).read_text(encoding='utf-8').splitlines()
    return selected_paths(root, result.stdout.decode('utf-8').split('\0'))


def input_hash(root=ROOT):
    root = Path(root)
    digest = hashlib.sha256()
    for name in input_paths(root):
        encoded = name.encode('utf-8')
        data = (root / name).read_bytes()
        digest.update(len(encoded).to_bytes(8, 'big'))
        digest.update(encoded)
        digest.update(len(data).to_bytes(8, 'big'))
        digest.update(data)
    return digest.hexdigest()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    print(input_hash(parser.parse_args().root))
