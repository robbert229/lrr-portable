#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Hash matching-build source inputs, independently of Git and the toolchain."""
import argparse
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def input_hash(root=ROOT):
    root = Path(root)
    paths = sorted([p for p in (root / 'src').rglob('*') if p.is_file()] +
                   [root / 'CMakeLists.txt', root / 'cross/msvc5-toolchain.cmake', root / 'tools/reccmp.sh'])
    digest = hashlib.sha256()
    for path in paths:
        name = path.relative_to(root).as_posix().encode('utf-8')
        data = path.read_bytes()
        digest.update(len(name).to_bytes(8, 'big'))
        digest.update(name)
        digest.update(len(data).to_bytes(8, 'big'))
        digest.update(data)
    return digest.hexdigest()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    print(input_hash(parser.parse_args().root))
