#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Function-attributed approximations of original translation-unit layout."""
import bisect
import collections
import re
import statistics
from pathlib import Path

from translation_units import read_rows, ROOT
import link_order

RANGES = ROOT / 'docs/coverage/translation-unit-ranges.csv'


def mask_operands(code, address, image_lo, image_hi):
    """Offsets occupied by image addresses and external relative branch targets."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    md = Cs(CS_ARCH_X86, CS_MODE_32); md.detail = True
    masked = set()
    for ins in md.disasm(code, address):
        for op in ins.operands:
            if op.type == X86_OP_MEM and image_lo <= (op.mem.disp & 0xffffffff) < image_hi:
                masked.update(range(ins.address - address + ins.disp_offset,
                                    ins.address - address + ins.disp_offset + ins.disp_size))
            if op.type == X86_OP_IMM:
                target = op.imm & 0xffffffff
                external_branch = (ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)) and not address <= target < address + len(code)
                if external_branch or image_lo <= target < image_hi:
                    masked.update(range(ins.address - address + ins.imm_offset,
                                        ins.address - address + ins.imm_offset + ins.imm_size))
    return masked


def identical(a, b, aa, ba, orig_image, rebuilt_image):
    if len(a) != len(b):
        return False
    masked = mask_operands(a, aa, *orig_image) & mask_operands(b, ba, *rebuilt_image)
    return all(x == y or i in masked for i, (x, y) in enumerate(zip(a, b)))


def pair_counts(values):
    return {'pairs': sum(a > b for i, a in enumerate(values) for b in values[i+1:]),
            'adjacent': sum(a > b for a, b in zip(values, values[1:]))}


def section(pe, addr):
    rva = addr - pe.OPTIONAL_HEADER.ImageBase
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
            return s.Name.rstrip(b'\0').decode('ascii')
    return None


def object_summary(path):
    pattern = re.compile(r'^\s*0001:[0-9a-fA-F]+\s+\S+\s+([0-9a-fA-F]{8})\s+f\s+(\S+)', re.M)
    seen = {}
    for addr, obj in pattern.findall(Path(path).read_text(errors='replace')):
        seen.setdefault(obj, int(addr, 16))
    return {'method': 'first public .text symbol per map object; objects without public symbols omitted',
            'objects': [{'object': name, 'first_public_address': hex(addr)} for name, addr in sorted(seen.items(), key=lambda x:x[1])]}


def list_summary(rows, entries):
    starts = {row["unit_id"]: int(row["text_start"], 16) for row in rows}
    values = [link_order.entry_address(e, starts) for e in entries if e.applies_to("winapi")]
    values = [v for v in values if v is not None]
    return {"method": "list order versus original unit starts and legacy median-address keys; tail omitted",
            "entries": len(values), **pair_counts(values)}


def measure(original, rebuilt, funcs, entries, entities, ranges, annotations, map_path, list_entries=None):
    rows = sorted(ranges, key=lambda r: int(r['text_start'], 16))
    starts = [int(r['text_start'], 16) for r in rows]
    def owner(addr):
        i = bisect.bisect_right(starts, addr) - 1
        return i if i >= 0 and addr < int(rows[i]['text_end'], 16) else None
    by_orig = {int(e['address'], 16): e for e in entries}
    by_entity = {e['orig']: e for e in entities if e['orig'] is not None and e['recomp'] is not None and e['type'] == 1}
    base = rebuilt.OPTIONAL_HEADER.ImageBase
    rebuilt_bytes = rebuilt.get_memory_mapped_image()
    obase = original.pe.OPTIONAL_HEADER.ImageBase
    orig_image = (obase, obase + original.pe.OPTIONAL_HEADER.SizeOfImage)
    recomp_image = (base, base + rebuilt.OPTIONAL_HEADER.SizeOfImage)
    groups = [[] for _ in rows]; unattributed = 0
    for f in funcs:
        i = owner(f.address)
        if i is None:
            unattributed += 1; continue
        e = by_orig.get(f.address); ent = by_entity.get(f.address)
        mapped = bool(e and e.get('recomp'))
        rebuilt_addr = int(e['recomp'], 16) if mapped else None
        size_known = bool(ent and mapped and rebuilt_addr == ent['recomp'] and ent['size'] is not None)
        size = f.end - f.address
        equal = False
        if size_known:
            off = rebuilt_addr - base
            b = bytes(rebuilt_bytes[off:off + ent['size']]) if off >= 0 else b''
            equal = identical(original.read(f.address, size), b, f.address, ent['recomp'], orig_image, recomp_image)
        groups[i].append((f.address, size, rebuilt_addr, ent['size'] if size_known else None, equal))
    known = {f.address for f in funcs}
    foreign = [(int(e['recomp'], 16), None) for e in entries if e.get('recomp') and int(e['address'], 16) not in known]
    all_mapped = sorted([(x[2], i) for i, g in enumerate(groups) for x in g if x[2] is not None] + foreign, key=lambda x: x[0])
    medians = [(i, statistics.median(x[2] for x in g if x[2] is not None)) for i, g in enumerate(groups) if any(x[2] is not None for x in g)]
    inv = pair_counts([v for _, v in medians])
    runs = collections.defaultdict(int); prev = None
    for _, i in all_mapped:
        if i is not None and i != prev: runs[i] += 1
        prev = i
    contiguous = 0; units = []; prefix_units = prefix_bytes = 0; prefix = True
    data_by_addr = {e['orig']: e for e in entities if e['orig'] is not None and e['recomp'] is not None and e['type'] in (2, 3, 4, 11)}
    data_groups = [collections.defaultdict(list) for _ in rows]; unknown_data = 0
    for addr in sorted(annotations):
        candidates = []
        for i, row in enumerate(rows):
            for key in ('rdata_range', 'data_range', 'string_range', 'bss_range'):
                if row[key]:
                    lo, hi = (int(x, 16) for x in row[key].split('-'))
                    if lo <= addr <= hi: candidates.append(i)
        candidates = set(candidates); e = data_by_addr.get(addr)
        if len(candidates) != 1 or not e:
            unknown_data += 1; continue
        sec = section(original.pe, addr); newsec = section(rebuilt, e['recomp'])
        if not sec or not newsec:
            unknown_data += 1; continue
        data_groups[next(iter(candidates))][sec].append((addr, e['recomp'], newsec))
    for i, (row, g) in enumerate(zip(rows, groups)):
        mapped = [x for x in g if x[2] is not None]
        ordered = [x[0] for x in sorted(mapped, key=lambda x:x[2])]
        contig = bool(mapped) and runs[i] == 1 and ordered == sorted(ordered)
        contiguous += contig
        size_exact = bool(g) and len(mapped) == len(g) and all(x[1] == x[3] for x in g)
        if prefix and size_exact:
            prefix_units += 1; prefix_bytes += int(row['text_end'], 16) - int(row['text_start'], 16)
        else: prefix = False
        comparisons = []
        for items in data_groups[i].values():
            if len(items) < 2: continue
            items.sort(); a, b, s = items[0]
            comparisons.extend(t == s and x - a == y - b for x, y, t in items[1:])
        total = sum(x[1] for x in g); exact = sum(x[1] for x in g if x[4])
        units.append({'unit_id': row['unit_id'], 'functions': len(g), 'mapped': len(mapped), 'original_function_bytes': total,
                      'identical_function_bytes': exact, 'identity_percent': round(100 * exact / total, 2) if total else None,
                      'contiguous': contig if mapped else None, 'size_exact': size_exact,
                      'data_offset_agreement': all(comparisons) if comparisons else None, 'data_offset_comparisons': len(comparisons)})
    total = sum(u['original_function_bytes'] for u in units); exact = sum(u['identical_function_bytes'] for u in units)
    mapped_units = len(medians)
    return {'unit_order_inversions': {'method': 'median mapped function address per unit; ties are not inversions', **inv, 'units': mapped_units},
            'unit_contiguity': {'method': 'one run of mapped functions in original function order; unmapped functions omitted', 'units': contiguous, 'eligible_units': mapped_units, 'percent': round(100 * contiguous / mapped_units, 2) if mapped_units else 0.0},
            'unit_relative_identity': {'method': 'whole function byte identity; image-address operands and external relative branches masked; missing functions count against original bytes', 'identical_bytes': exact, 'original_bytes': total, 'percent': round(100 * exact / total, 2) if total else 0.0},
            'size_exact_prefix': {'method': 'all original functions mapped with equal code sizes; bytes use original unit spans', 'units': prefix_units, 'bytes': prefix_bytes, 'size_exact_units': sum(u['size_exact'] for u in units)},
            'data_offset_agreement': {'method': 'anchored: compare mapped annotated offsets within each original evidence range and PE section; uncertain ownership omitted', 'agreeing_units': sum(u['data_offset_agreement'] is True for u in units), 'eligible_units': sum(u['data_offset_agreement'] is not None for u in units), 'unattributed_or_unmapped_annotations': unknown_data},
            'unattributed_mapped_functions': len(foreign), 'unattributed_original_functions': unattributed, 'unmapped_original_functions': sum(len(g) - sum(x[2] is not None for x in g) for g in groups),
            'units': units, 'object_map': object_summary(map_path),
            'link_order_list': list_summary(rows, list_entries if list_entries is not None else link_order.load())}


def annotation_addresses(root=ROOT):
    pattern = re.compile(r'^\s*//\s*(?:GLOBAL|STRING):\s*LEGORR\s+0x([0-9a-f]+)', re.I | re.M)
    return {int(a, 16) for p in (root / 'src').rglob('*') if p.suffix in ('.c', '.cpp', '.h', '.hpp') for a in pattern.findall(p.read_text(encoding='utf-8', errors='replace'))}
