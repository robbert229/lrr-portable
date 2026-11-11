#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Bounded, conservative source search using the single-function match loop."""
import argparse
import concurrent.futures
import difflib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent


def mask(text):
    """Hide comments and literals without changing offsets or newlines."""
    pattern = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: re.sub(r'[^\n]', ' ', m.group()), text)


def function_span(text, address):
    matches = list(re.finditer(r'//\s*FUNCTION\s*:\s*LEGORR\s+(0x[0-9a-fA-F]+)\b', text))
    found = [m for m in matches if int(m.group(1), 16) == address]
    if len(found) != 1:
        raise ValueError('expected exactly one FUNCTION annotation')
    start = found[0].end()
    code = mask(text)
    opening = code.find('{', start)
    if opening < 0 or re.search(r';|#', code[start:opening]):
        raise ValueError('cannot safely locate function body')
    depth = 1
    for i in range(opening + 1, len(code)):
        if code[i] == '{':
            depth += 1
        elif code[i] == '}':
            depth -= 1
            if not depth:
                return opening + 1, i
    raise ValueError('unterminated function')


def rewrites(text, address, c_language=True):
    """Yield one-edit candidates; avoid parsing constructs we cannot prove safe."""
    start, end = function_span(text, address)
    code = mask(text)
    body = code[start:end]
    edits = []
    # Invert an integer-literal comparison. Only one operand is evaluated.
    comparison = r'\b([A-Za-z_]\w*)\s*(==|!=|<=|>=|<|>)\s*(0[xX][0-9a-fA-F]+[uUlL]*|[0-9]+[uUlL]*)\b(?=\s*(?:\)|&&|\|\|))'
    reverse = {'==': '==', '!=': '!=', '<': '>', '>': '<', '<=': '>=', '>=': '<='}
    for m in re.finditer(comparison if c_language else r'(?!)', body):
        # Do not rewrite a member, subexpression, float, or part of an operator.
        a, b = start + m.start(), start + m.end()
        prior = code[start:a].rstrip()
        if text[a:b] != code[a:b] or not (prior.endswith('(') or prior.endswith('&&') or prior.endswith('||')):
            continue
        edits.append((a, b, f'{m[3]} {reverse[m[2]]} {m[1]}', 'comparison'))
    # while (condition) is exactly for (; condition ;) including continue.
    for m in re.finditer(r'\bwhile\s*\(', body):
        a = start + m.start()
        pos = start + m.end()
        depth = 1
        i = pos
        while i < end and depth:
            depth += (code[i] == '(') - (code[i] == ')')
            i += 1
        # Exclude do-while tails conservatively: only accept line-start while.
        if depth == 0 and not code[code.rfind('\n', 0, a)+1:a].strip():
            prior = code[:a].rstrip()
            if not prior.endswith('{'):
                continue
            edits.append((a, i, 'for (;' + text[pos:i-1] + ';)', 'loop'))
    # Adjacent, uninitialized primitive declarations have no runtime effects.
    decl = r'^([ \t]*(?:unsigned |signed )?(?:int|long|short|char|float|double)\s+[A-Za-z_]\w*\s*;[ \t]*\n)'
    for m in re.finditer(decl + decl, body, re.M):
        a, b = start + m.start(), start + m.end()
        edits.append((a, b, text[start+m.start(2):start+m.end(2)] + text[start+m.start(1):start+m.end(1)], 'declarations'))
    # Swap adjacent names in a primitive declaration with no initializers.
    for m in re.finditer(r'(?m)^[ \t]*(?:unsigned |signed )?(?:int|long|short|char|float|double)[ \t]+([A-Za-z_]\w*(?:[ \t]*,[ \t]*[A-Za-z_]\w*)+)[ \t]*;', body):
        a, b = start + m.start(1), start + m.end(1)
        names = re.split(r'[ \t]*,[ \t]*', text[a:b])
        for i in range(len(names) - 1):
            swapped = names[:]
            swapped[i], swapped[i + 1] = swapped[i + 1], swapped[i]
            edits.append((a, b, ', '.join(swapped), 'declarators'))
    # Scalar locals only: no pointers, initializers, volatile qualifiers or
    # user-defined types. Restrict arithmetic to unsigned to avoid overflow
    # and floating-point reassociation differences.
    locals_by_type = {}
    for m in re.finditer(r"(?m)^[ \t]*(unsigned int|int|long|short|char)[ \t]+([A-Za-z_]\w*(?:[ \t]*,[ \t]*[A-Za-z_]\w*)*)[ \t]*;", body):
        for name in re.split(r"[ \t]*,[ \t]*", m[2]):
            locals_by_type[name] = m[1]
    # Without a C parser we cannot resolve shadowing in nested scopes.
    # Do not infer local types in functions that contain nested blocks, except
    # when recognizing the dedicated inner-block temporary pattern below.
    if '{' in body or '}' in body:
        locals_by_type = {}
    # Only bare scalar assignments with distinct destinations and RHS reads
    # from known locals. This excludes calls, memory aliases and volatile data.
    assignment = r"([ \t]*([A-Za-z_]\w*)[ \t]*=[ \t]*([A-Za-z_]\w*|[0-9]+)[ \t]*;[ \t]*\n)"
    for m in re.finditer(r"(?m)^" + assignment + r"^" + assignment, body):
        dst1, rhs1, dst2, rhs2 = m[2], m[3], m[5], m[6]
        if dst1 not in locals_by_type or dst2 not in locals_by_type or dst1 == dst2:
            continue
        if any(not x.isdigit() and x not in locals_by_type for x in (rhs1, rhs2)):
            continue
        if dst1 == rhs2 or dst2 == rhs1:
            continue
        a, b = start + m.start(), start + m.end()
        edits.append((a, b, text[start+m.start(4):start+m.end(4)] + text[start+m.start(1):start+m.end(1)], 'statements'))
    # Introduce a same-type temporary in an inner block around a return.
    for m in re.finditer(r"\breturn[ \t]+([A-Za-z_]\w*)[ \t]*;", body):
        name = m[1]
        if name not in locals_by_type or re.search(r"\b_perm_tmp\b", code):
            continue
        a, b = start + m.start(), start + m.end()
        edits.append((a, b, '{ ' + locals_by_type[name] + ' _perm_tmp = ' + name + '; return _perm_tmp; }', 'temporary-introduce'))
    # A temporary of the function's scalar return type preserves the return
    # conversion and evaluates the original expression exactly once. Do not
    # use floating types: storing them can force x87 rounding.
    marker = re.search(r'//\s*FUNCTION\s*:\s*LEGORR\s+' + re.escape(hex(address)) + r'\b', text, re.I)
    if marker is None:
        marker = next(m for m in re.finditer(r'//\s*FUNCTION\s*:\s*LEGORR\s+(0x[0-9a-fA-F]+)', text) if int(m[1], 16) == address)
    signature = code[marker.end():start - 1]
    sig = re.search(r'\b(unsigned int|int|uint32_t|int32_t|unsigned long|long)\s+[A-Za-z_]\w*\s*\([^;{}]*\)\s*$', signature)
    if sig and '_perm_tmp' not in code:
        for m in re.finditer(r'\breturn[ \t]+([^;{}\n]+);', body):
            a, b = start + m.start(), start + m.end()
            expr_text = text[start+m.start(1):start+m.end(1)]
            edits.append((a, b, '{ ' + sig[1] + ' _perm_tmp = (' + expr_text + '); return _perm_tmp; }', 'return-temporary'))
    # Remove a temporary only when source and temporary have the same type.
    for m in re.finditer(r"\{[ \t]*(unsigned int|int|long|short|char)[ \t]+([A-Za-z_]\w*)[ \t]*=[ \t]*([A-Za-z_]\w*)[ \t]*;[ \t]*return[ \t]+\2[ \t]*;[ \t]*\}", body):
        before_block = body[:m.start()]
        outer_decl = re.search(r'(?m)^[ \t]*' + re.escape(m[1]) + r'[ \t]+' + re.escape(m[3]) + r'[ \t]*;', before_block)
        if not outer_decl or '{' in before_block or '}' in before_block:
            continue
        a, b = start + m.start(), start + m.end()
        edits.append((a, b, 'return ' + m[3] + ';', 'temporary-remove'))
    # Reassociation/commutation are safe here for unsigned integers, and only
    # within a complete return expression (not an arbitrary subexpression).
    expr = r"\breturn[ \t]+\(?([A-Za-z_]\w*)[ \t]*([+*&|^])[ \t]*([A-Za-z_]\w*)\)?[ \t]*;"
    for m in re.finditer(expr, body):
        if not all(locals_by_type.get(x) == 'unsigned int' for x in (m[1], m[3])):
            continue
        a, b = start + m.start(), start + m.end()
        edits.append((a, b, 'return ' + m[3] + ' ' + m[2] + ' ' + m[1] + ';', 'commutation'))
    assoc = r"\breturn[ \t]+\([ \t]*([A-Za-z_]\w*)[ \t]*([+*&|^])[ \t]*([A-Za-z_]\w*)[ \t]*\)[ \t]*\2[ \t]*([A-Za-z_]\w*)[ \t]*;"
    for m in re.finditer(assoc, body):
        if not all(locals_by_type.get(x) == 'unsigned int' for x in (m[1], m[3], m[4])):
            continue
        a, b = start + m.start(), start + m.end()
        edits.append((a, b, f'return {m[1]} {m[2]} ({m[3]} {m[2]} {m[4]});', 'association'))
    # Empty-init/step for and while have identical continue behavior.
    for m in re.finditer(r"\bfor[ \t]*\([ \t]*;[ \t]*([A-Za-z_]\w*)[ \t]*;[ \t]*\)", body):
        a, b = start + m.start(), start + m.end()
        edits.append((a, b, 'while (' + m[1] + ')', 'for-to-while'))
    # A guarded do/while evaluates the condition at the same points as while.
    for m in re.finditer(r"\bwhile[ \t]*\([ \t]*([A-Za-z_]\w*)[ \t]*\)[ \t]*\{([^{}#]*)\}", body):
        a, b = start + m.start(), start + m.end()
        prefix = code[:a].rstrip()
        if not prefix.endswith('{') or re.search(r"\b(?:goto|case|default)\b|:", m[2]):
            continue
        content = text[start+m.start(2):start+m.end(2)]
        edits.append((a, b, '{ if (' + m[1] + ') do {' + content + '} while (' + m[1] + '); }', 'while-to-do'))
    # Restrict do/while conversion to simple braces and no preprocessor or
    # labels; the flag preserves continue, break and the first iteration.
    for m in re.finditer(r"\bdo[ \t]*\{([^{}#]*)\}[ \t]*while[ \t]*\([ \t]*([A-Za-z_]\w*)[ \t]*\)[ \t]*;", body):
        if re.search(r"\b(?:goto|case|default)\b|:", m[1]) or re.search(r"\b_perm_first\b", code):
            continue
        a, b = start + m.start(), start + m.end()
        content = text[start+m.start(1):start+m.end(1)]
        edits.append((a, b, '{ int _perm_first = 1; for (; _perm_first || ' + m[2] + '; _perm_first = 0) {' + content + '} }', 'do-to-for'))
    for a, b, replacement, kind in edits:
        yield kind, text[:a] + replacement + text[b:]


def score(address, deadline, candidate=None):
    cmd = [sys.executable, str(ROOT / 'tools/match_function.py'), hex(address), '--json', '--no-diff']
    if candidate:
        cmd += ['--candidate', str(candidate)]
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise TimeoutError('time budget exhausted')
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True)
    try:
        out, err = p.communicate(timeout=remaining)
    except subprocess.TimeoutExpired:
        os.killpg(p.pid, signal.SIGKILL)
        p.communicate()
        raise TimeoutError('time budget exhausted')
    try:
        doc = json.loads(out)
        result = doc['functions'][0]
        if p.returncode or result.get('error') or result.get('matching') is None:
            raise ValueError(result.get('error') or str(doc))
        return result
    except (ValueError, KeyError, IndexError) as ex:
        raise ValueError(f'scoring failed: {ex}; {err.decode(errors="replace")[-1000:]}') from ex


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('address', type=lambda x: int(x, 16))
    parser.add_argument('--seconds', type=float, required=True, help='total time budget, including initial score')
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--output', type=Path, required=True, help='output directory for best source, patch and results')
    args = parser.parse_args()
    if args.seconds <= 0 or args.workers < 1:
        parser.error('seconds and workers must be positive')
    deadline = time.monotonic() + args.seconds
    try:
        initial = score(args.address, deadline)
        source = ROOT / initial['source']
        original = source.read_text()
        best = original
        best_score = initial['matching']
        args.output.mkdir(parents=True, exist_ok=True)
        seen = {original}
        attempts = 0
        failures = []
        with tempfile.TemporaryDirectory(prefix='permuter-', dir=args.output) as tmp:
            with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
                while time.monotonic() < deadline and best_score < 1:
                    try:
                        variants = [(kind, candidate) for kind, candidate in rewrites(best, args.address, source.suffix == '.c') if candidate not in seen]
                    except ValueError as ex:
                        failures.append(str(ex))
                        break
                    if not variants:
                        break
                    improved = False
                    for offset in range(0, len(variants), args.workers):
                        if time.monotonic() >= deadline:
                            break
                        batch = variants[offset:offset + args.workers]
                        futures = []
                        for index, (kind, candidate) in enumerate(batch):
                            seen.add(candidate)
                            path = Path(tmp) / f'{index}{source.suffix}'
                            path.write_text(candidate)
                            futures.append((kind, candidate, pool.submit(score, args.address, deadline, path.resolve())))
                        for kind, candidate, future in futures:
                            attempts += 1
                            try:
                                result = future.result()
                                if result['matching'] > best_score:
                                    best, best_score, improved = candidate, result['matching'], True
                                    print(f'{kind}: {best_score * 100:.2f}%', flush=True)
                            except (ValueError, TimeoutError) as ex:
                                failures.append(str(ex))
                    if not improved:
                        break
        (args.output / ('best' + source.suffix)).write_text(best)
        patch = ''.join(difflib.unified_diff(original.splitlines(True), best.splitlines(True), fromfile='a/' + initial['source'], tofile='b/' + initial['source']))
        (args.output / 'best.patch').write_text(patch)
        data = {'address': hex(args.address), 'source': initial['source'], 'before': initial['matching'], 'best': best_score, 'attempts': attempts, 'failures': failures}
        (args.output / 'result.json').write_text(json.dumps(data, indent=2) + '\n')
        print(json.dumps(data, indent=2))
    except (ValueError, TimeoutError, OSError) as ex:
        print(f'permuter: {ex}', file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
