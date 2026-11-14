#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Identity metrics toward a byte-identical image (issue #793, finding 668).

`binary_progress.py` records these in `binary-progress.json` (schema 5):

- `correct_size_functions`: mapped functions whose rebuilt code size equals
  the original's, for the full and game-only Reccmp scopes.
- `identical_prefix`: bytes of `.text` identical at the original addresses
  from the section start up to the first differing byte (and, supplementary,
  the same with address operands masked).
- `unit_size_agreement`: per original unit, whether every listed function has
  the correct size and the unit's first and last functions start and end at
  the original addresses.

Sizes come from `code_extent`, applied with the same rule to both images.
"""
import bisect
import re


PADDING = (0x90, 0xCC)


def code_extent(read, start, limit):
    """Bytes from `start` that belong to the function: reachable code plus tables.

    Reachable code follows `translation_units.load_functions`: instructions
    reached from `start` through jumps that stay below `limit` (the next known
    function start), stopping at `ret`, `jmp`, `int3` and `hlt`, and through
    in-function jump tables. In-function data after the code (switch jump and
    index tables) is referenced by a memory operand of that code; the extent
    then runs to the first 16-byte boundary past the last table that follows a
    padding byte (`nop` or `int3`), or to `limit`, without trailing padding.
    The same rule measures both images, so a CRT member's alignment and a
    switch table count the same way on each side.
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    code = read(start, limit - start)
    seen, work, end, tables = set(), [start], start, []
    while work:
        a = work.pop()
        while start <= a < limit and a not in seen:
            ins = next(md.disasm(code[a - start:a - start + 16], a), None)
            if ins is None:
                break
            seen.add(a)
            end = max(end, a + ins.size)
            tables += [op.mem.disp & 0xFFFFFFFF for op in ins.operands if op.type == X86_OP_MEM]
            m = ins.mnemonic
            op = ins.operands[0] if ins.operands else None
            if m.startswith('j') and op is not None and op.type == X86_OP_IMM:
                if start <= op.imm < limit:
                    work.append(op.imm)
                if m == 'jmp':
                    break
            elif m == 'jmp':
                if op is not None and op.type == X86_OP_MEM and start <= op.mem.disp < limit:
                    p = op.mem.disp
                    while start <= p < limit and p + 4 <= limit:
                        t = int.from_bytes(code[p - start:p - start + 4], 'little')
                        if not start <= t < limit:
                            break
                        work.append(t)
                        p += 4
                break
            if m in ('ret', 'retn', 'int3', 'hlt'):
                break
            a += ins.size
    tables = [t for t in tables if end <= t < limit]
    if not tables:
        return end - start
    boundary = (max(tables) // 16 + 1) * 16
    while boundary < limit and code[boundary - 1 - start] not in PADDING:
        boundary += 16
    n = min(boundary, limit) - start
    while n > end - start and code[n - 1] in PADDING:
        n -= 1
    return n


def map_function_starts(map_path):
    """Every `.text` symbol address (publics and statics) in a LINK map."""
    pattern = re.compile(r'^\s*0001:[0-9a-fA-F]+\s+\S+\s+([0-9a-fA-F]{8})\s', re.M)
    return {int(a, 16) for a in pattern.findall(map_path.read_text(errors='replace'))}


def function_sizes(entries, entities, original, rebuilt):
    """Original and rebuilt `code_extent` of each mapped function.

    `original` and `rebuilt` are `(read, starts, text_end)`: a byte reader by
    virtual address, the known function starts (each extent stops at the next
    one) and the end of `.text`. Original starts: the listed functions and
    every Reccmp-mapped address. Rebuilt starts: the map's `.text` symbols and
    the matched entities. Keyed by original address; an entry without a
    matched function entity (`entities.json` type 1) is omitted.
    """
    pairs = {e['orig']: e['recomp'] for e in entities
             if e.get('type') == 1 and e.get('orig') is not None and e.get('recomp') is not None}
    sizes = {}
    original, rebuilt = ((r, sorted(s), end) for r, s, end in (original, rebuilt))
    def extent(side, address):
        read, starts, end = side
        i = bisect.bisect_right(starts, address)
        return code_extent(read, address, starts[i] if i < len(starts) else end)
    for e in entries:
        address = int(e['address'], 16)
        if not e.get('recomp') or pairs.get(address) != int(e['recomp'], 16) or address in sizes:
            continue
        sizes[address] = (extent(original, address), extent(rebuilt, pairs[address]))
    return sizes


def correct_size(entries, sizes):
    """Mapped functions of one Reccmp scope whose rebuilt size equals the original's.

    Bytes are original bytes. `uncompared` counts mapped entries without a
    size pair. Equal size is not equal code.
    """
    mapped = compared = count = size_bytes = exact = 0
    for e in entries:
        if not e.get('recomp'):
            continue
        mapped += 1
        exact += e.get('matching') == 1.0
        pair = sizes.get(int(e['address'], 16))
        if pair is None:
            continue
        compared += 1
        if pair[0] == pair[1]:
            count += 1
            size_bytes += pair[0]
    return {'count': count, 'bytes': size_bytes, 'compared': compared,
            'uncompared': mapped - compared, 'mapped': mapped, 'exact_functions': exact}


def unit_size_agreement(rows, funcs, entries, sizes):
    """Per original unit: correct sizes and agreeing start and end addresses.

    A unit's functions are the listed original functions inside its
    `text_start`-`text_end` range. Correct size needs every one mapped with
    equal extents. The start agrees when the first function sits at its
    original address; the end agrees when the last function's rebuilt code
    ends at its original end address. Code ends, not unit spans: padding
    after the last function is not compared. Units without listed functions
    are omitted.
    """
    recomp = {int(e['address'], 16): int(e['recomp'], 16) for e in entries if e.get('recomp')}
    ordered = sorted(f.address for f in funcs)
    units = []
    for row in sorted(rows, key=lambda r: int(r['text_start'], 16)):
        lo, hi = int(row['text_start'], 16), int(row['text_end'], 16)
        members = ordered[bisect.bisect_left(ordered, lo):bisect.bisect_left(ordered, hi)]
        if not members:
            continue
        correct = all(a in sizes and sizes[a][0] == sizes[a][1] for a in members)
        first, last = members[0], members[-1]
        start = recomp.get(first) == first
        end = last in sizes and last in recomp and recomp[last] + sizes[last][1] == last + sizes[last][0]
        units.append({'unit_id': row['unit_id'], 'functions': len(members), 'correct_size': correct,
                      'start_agrees': start, 'end_agrees': end, 'agrees': correct and start and end})
    return {'units': sum(u['agrees'] for u in units), 'eligible_units': len(units),
            'correct_size_units': sum(u['correct_size'] for u in units),
            'start_agreeing_units': sum(u['start_agrees'] for u in units),
            'end_agreeing_units': sum(u['end_agrees'] for u in units),
            'agreeing_unit_ids': [u['unit_id'] for u in units if u['agrees']],
            'per_unit': units}


def _attribute(result, address, funcs):
    ordered = sorted(funcs, key=lambda f: f.address)
    i = bisect.bisect_right([f.address for f in ordered], address) - 1
    f = ordered[i] if i >= 0 else None
    result.update(first_difference=hex(address), function=getattr(f, 'name', None) if f else None,
                  function_address=hex(f.address) if f else None,
                  offset_in_function=address - f.address if f else None)
    return result


def _prefix(n, total):
    return {'bytes': n, 'original_text_bytes': total, 'percent': round(100 * n / total, 2) if total else 0.0}


def masked_prefix(original, rebuilt, start, funcs, orig_image, rebuilt_image):
    """The identical prefix with address operands masked, function by function.

    Inside each listed original function's code extent, bytes that both sides
    disassemble as image-address operands or external relative branch targets
    (`layout_progress.mask_operands`) count as equal; padding between functions
    and code outside the listed extents compare raw. It shows how far the code
    agrees apart from where data and callees landed; it is not identity.
    """
    from layout_progress import mask_operands
    limit = min(len(original), len(rebuilt))
    n = 0
    for f in sorted(funcs, key=lambda f: f.address):
        lo, hi = f.address - start, min(f.end - start, limit)
        if lo < n or lo >= limit:
            continue
        while n < lo and original[n] == rebuilt[n]:
            n += 1
        if n < lo:
            break
        a, b = bytes(original[lo:hi]), bytes(rebuilt[lo:hi])
        masked = None
        while n < hi:
            if original[n] != rebuilt[n]:
                if masked is None:
                    masked = mask_operands(a, f.address, *orig_image) & mask_operands(b, f.address, *rebuilt_image)
                if n - lo not in masked:
                    break
            n += 1
        if n < hi:
            break
    else:
        while n < limit and original[n] == rebuilt[n]:
            n += 1
    result = _prefix(n, len(original))
    if n == len(original):
        result.update(first_difference=None, function=None, function_address=None, offset_in_function=None)
        return result
    return _attribute(result, start + n, funcs)


def identical_prefix(original, rebuilt, start, funcs):
    """Identical bytes at the original addresses from `start` to the first difference.

    `original` is the original `.text` (virtual size) and `rebuilt` the
    rebuilt image's bytes at the same virtual addresses (whatever section is
    mapped there). Raw bytes: no operand masking. A rebuilt image that ends
    early differs at its end. The first differing address is attributed to the
    original function whose start is at or below it (`None` before the first).
    """
    n = 0
    limit = min(len(original), len(rebuilt))
    while n < limit and original[n] == rebuilt[n]:
        n += 1
    result = _prefix(n, len(original))
    if n == len(original):
        result.update(first_difference=None, function=None, function_address=None, offset_in_function=None)
        return result
    return _attribute(result, start + n, funcs)


def measure(original, rebuilt, map_path, funcs, full, game, entities, rows):
    """The identity metrics of one matching build, as `binary-progress.json` keys.

    `original` is a `translation_units.Binary`, `rebuilt` a `pefile.PE`.
    """
    text = original.read(original.text[0], original.text[1] - original.text[0])
    base = rebuilt.OPTIONAL_HEADER.ImageBase
    image = rebuilt.get_memory_mapped_image()
    def rebuilt_read(address, size):
        return bytes(image[address - base:address - base + size]) if address >= base else b''
    at_original = rebuilt_read(original.text[0], len(text))
    prefix = identical_prefix(text, at_original, original.text[0], funcs)
    obase = original.pe.OPTIONAL_HEADER.ImageBase
    prefix['address_operands_masked'] = masked_prefix(
        text, at_original, original.text[0], funcs,
        (obase, obase + original.pe.OPTIONAL_HEADER.SizeOfImage), (base, base + rebuilt.OPTIONAL_HEADER.SizeOfImage))
    section = next(s for s in rebuilt.sections if s.Name.rstrip(b'\0') == b'.text')
    rebuilt_starts = {e['recomp'] for e in entities if e.get('type') == 1 and e.get('recomp') is not None}
    if map_path.is_file():
        rebuilt_starts |= map_function_starts(map_path)
    sizes = function_sizes(
        full, entities,
        (original.read, {f.address for f in funcs} | {int(e['address'], 16) for e in full}, original.text[1]),
        (rebuilt_read, rebuilt_starts, base + section.VirtualAddress + section.Misc_VirtualSize))
    return {'identical_prefix': prefix,
            'correct_size_functions': {'method': 'code_extent on both images (reachable code plus in-function tables, '
                                                 'up to the next known function start, without trailing padding); '
                                                 'bytes are original bytes',
                                       'full': correct_size(full, sizes), 'game_only': correct_size(game, sizes)},
            'unit_size_agreement': {'method': 'listed original functions in each unit range: all mapped with correct size, '
                                              'first function at its original address, last function code ending at '
                                              'its original end; trailing padding not compared',
                                    **unit_size_agreement(rows, funcs, full, sizes)}}
