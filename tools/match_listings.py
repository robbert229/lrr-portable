#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Fingerprints of rebuilt Reccmp listings and annotated function source."""
import hashlib
import json
import re
import subprocess
from pathlib import Path

SCOPES = (('game-only', 'game-only.json'), ('full', 'results.json'))
DEFAULT = 'docs/coverage/match-listing-digests.txt'
FORMAT = '# listing_format: 4'
REGISTER = re.compile(r'\b(?:e(?:ax|bx|cx|dx|si|di|bp|sp)|(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][hl]|st(?:\([0-7]\))?)\b', re.I)
BRANCH = re.compile(r'^(?:j\w+|loop\w*)\s+.*$', re.I)
STACK = re.compile(r'\b(?:esp|ebp|sp|bp)\s*([+-])\s*(?:0x[0-9a-f]+|\d+)', re.I)
ABSOLUTE = re.compile(r'\b0x[0-9a-f]{5,}\b', re.I)


def normalize(instruction):
    text = instruction.split('\t', 1)[0].strip().lower()
    text = re.sub(r'\s+', ' ', text)
    if BRANCH.match(text):
        return text.split(' ', 1)[0] + ' <target>'
    # Symbol names and relocation placeholders stand for addresses. Keep the
    # surrounding brackets, registers and explicit size in the instruction.
    # Mangled file-local names can contain a path and compilation number.
    text = re.sub(r'\?[^\s,]+\s*\((?:data|function|offset)\)', '<address>', text)
    # MSVC's anonymous namespace identifier changes between compilations.
    text = re.sub(r'\?a0x[0-9a-f]+::[^\s,\]+]+(?:\s*\((?:data|function|offset)\))?', '<address>', text)
    text = re.sub(r"thunk of '[^']*'", '<address>', text)
    text = re.sub(r'[-+]?\d+(?:\.\d+)?\s*\(float\)', '<address>', text)
    text = re.sub(r'<offset\d+>', '<address>', text)
    text = re.sub(r"(?:[a-z_$?][\w$?:.@]*(?:\[\d+\]|\.[a-z_$][\w$]*)*(?:\s*\+\s*(?:0x[0-9a-f]+|\d+))?|\"[^\"]*\")\s*\((?:data|function|import_thunk|offset)\)", '<address>', text)
    text = re.sub(r"(?<![\w])(?!(?:e(?:ax|bx|cx|dx|si|di|bp|sp)|(?:ax|bx|cx|dx|si|di|bp|sp))\b)[a-z_$?][\w$?:.@]*(?:\[\d+\])*\s*\+\s*0x[0-9a-f]+", '<address>', text)
    text = re.sub(r'"(?:\\.|[^"\\])*"', '<address>', text)
    text = re.sub(r'(?<![\w])(?:0x[0-9a-f]+|\d+)\s*\(offset\)', '<address>', text)
    text = STACK.sub(lambda m: 'stack ' + m.group(1) + ' offset', text)
    def mask_register(match):
        reg = match.group().lower()
        if reg.startswith('st'):
            return 'fpreg'
        if len(reg) == 3:
            return 'reg32'
        return 'reg8' if reg[1] in 'hl' else 'reg16'
    text = REGISTER.sub(mask_register, text)
    return ABSOLUTE.sub('<address>', text)


def fingerprint(lines):
    normalized = [normalize(line) for line in lines]
    def digest(items):
        return hashlib.sha256(json.dumps(items, separators=(',', ':')).encode()).hexdigest()
    return len(normalized), digest(normalized), digest(sorted(normalized))


def report_listings(directory):
    result = {}
    for scope, filename in SCOPES:
        data = json.loads((Path(directory) / filename).read_text())
        entries = data['data'] if isinstance(data, dict) else data
        listings = {}
        for entry in entries:
            if 'diff' not in entry:
                raise ValueError('report lacks complete diff for %s; regenerate with make reccmp-report' % entry['address'])
            lines = []
            for _, groups in entry['diff']:
                for group in groups:
                    if 'both' in group:
                        lines.extend(row[1] for row in group['both'] if row[2])
                    else:
                        lines.extend(row[1] for row in group.get('recomp', []) if row[0])
            if not lines:
                raise ValueError('empty rebuilt listing for %s' % entry['address'])
            listings[int(str(entry['address']), 16)] = fingerprint(lines)
        result[scope] = listings
    return result


def render(scopes, input_digest):
    lines = ['# input_sha256: ' + input_digest, FORMAT, '# Normalised rebuilt listings: address count sequence_sha256 multiset_sha256']
    for scope, _ in SCOPES:
        lines.append('[' + scope + ']')
        for address, (count, seq, bag) in sorted(scopes[scope].items()):
            lines.append('0x%08x %d %s %s' % (address, count, seq, bag))
    return '\n'.join(lines) + '\n'


def load(path):
    result = {scope: {} for scope, _ in SCOPES}
    current = None
    content = Path(path).read_text().splitlines()
    if FORMAT not in content:
        raise ValueError('unsupported listing digest format')
    for line in content:
        if not line or line.startswith('#'):
            continue
        if line.startswith('['):
            current = line[1:-1]
            if current not in result:
                raise ValueError('unknown listing scope ' + current)
        else:
            address, count, seq, bag = line.split()
            if current is None or not re.fullmatch('[0-9a-f]{64}', seq) or not re.fullmatch('[0-9a-f]{64}', bag):
                raise ValueError('invalid listing digest')
            result[current][int(address, 16)] = (int(count), seq, bag)
    return result


def classify(before, after, changed):
    delta = after[0] - before[0]
    if changed is None:
        return 'source-unknown', delta
    if changed:
        return 'source-changed', delta
    if before[1] == after[1]:
        return 'drift', delta
    if before[2] == after[2]:
        return 'drift-reordered', delta
    return 'drift-review', delta


def source_index(ref=None, root=Path.cwd()):
    """Index marked function bodies in a git tree or the current worktree.

    Missing or ambiguous annotations are deliberately unknown, never unchanged.
    """
    cmd = ['git', '-C', str(root), 'grep', '-l', '-i', '-E', r'//[[:space:]]*(FUNCTION|LIBRARY):[[:space:]]*LEGORR[[:space:]]+0x[0-9a-f]+']
    if ref:
        cmd.append(ref)
    cmd += ['--', 'src']
    result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode not in (0, 1):
        raise ValueError('cannot read source at %s: %s' % (ref or 'worktree', result.stderr.strip()))
    index = {}
    marker = re.compile(r'//\s*(FUNCTION|LIBRARY):\s*LEGORR\s+0x([0-9a-f]+)[^\n]*\n', re.I)
    # Ignore braces in strings and comments when locating a function body.
    token = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{};]', re.S)
    for name in result.stdout.splitlines():
        path = name.split(':', 1)[1] if ref else name
        if ref:
            proc = subprocess.run(['git', '-C', str(root), 'show', ref + ':' + path], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            text = proc.stdout
        else:
            text = (Path(root) / path).read_text(encoding='utf-8')
        for match in marker.finditer(text):
            start = match.end()
            depth = 0
            end = None
            for t in token.finditer(text, start):
                c = t.group()
                if c == '{':
                    depth += 1
                elif c == '}' and depth:
                    depth -= 1
                    if depth == 0:
                        end = t.end()
                        break
                elif c == ';' and depth == 0:
                    break
            address = int(match.group(2), 16)
            value = '<library>' if match.group(1).upper() == 'LIBRARY' else (text[start:end] if end else None)
            if address in index:
                index[address] = None
            else:
                index[address] = value
    return index
