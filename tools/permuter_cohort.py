#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Identify report diffs with the same normalized instruction multiset."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

REGISTER = re.compile(r'\b(?:e(?:ax|bx|cx|dx|si|di|bp|sp)|(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][hl]|st(?:\([0-7]\))?)\b', re.I)
NUMBER = re.compile(r'(?<![\w.])(?:-?0x[0-9a-f]+|-?\d+)(?![\w.])', re.I)


def normalize(instruction):
    instruction = instruction.split('\t', 1)[0].strip().lower()
    instruction = REGISTER.sub('REG', instruction)
    instruction = NUMBER.sub('NUM', instruction)
    return re.sub(r'\s+', ' ', instruction)


def classify(entry):
    if entry.get('matching', 0) >= 1 or entry.get('effective') or not entry.get('diff'):
        return False
    orig, recomp = [], []
    for _, groups in entry['diff']:
        for group in groups:
            orig.extend(normalize(row[1]) for row in group.get('orig', []))
            recomp.extend(normalize(row[1]) for row in group.get('recomp', []))
    return bool(orig) and Counter(orig) == Counter(recomp)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    data = json.loads(args.report.read_text())['data']
    result = [{'address': e['address'], 'name': e.get('name'), 'matching': e.get('matching')} for e in data if classify(e)]
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
