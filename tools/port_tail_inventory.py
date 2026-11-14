#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Inventory of the matching build's port tail (issue #793, finding 668).

Reads an existing matching build (`build-reccmp/`: the executable, its map,
the Reccmp report and the objects listed in `link-members.txt`) and reports:

- `layout`: the rebuilt `.text` as runs of one link-order kind (units, LIBC,
  import thunks, the tail), and where the entry point falls;
- `entry_point`: the entry point's offset from the original, split into the
  unit segment's end (the original's first CRT address) and the LIBC bytes
  before `WinMainCRTStartup`;
- `unit_spans`: each original unit's rebuilt object span against its
  original text range (`translation-unit-ranges.csv`); a unit object's span
  runs from its first `.text` symbol to the next object's;
- `objects`: for each object linked from `lrr_tail.lib` (the port tail and
  the CRT-range legacy file), its bytes per section, who references it and
  what it references. `.text` bytes come from the map (each symbol owns the
  bytes up to the next one). Data bytes come from the object's COFF
  sections: every non-COMDAT data section, and each COMDAT (string literals)
  only when the map attributes its symbol to this object. `referenced_by`
  counts references from code (disassembly) and initialised data (aligned
  dwords equal to a symbol's start) of non-tail objects, by unit id or LIBC; `references_units` counts
  the object's own references into unit objects. `stands_in_for` pairs, in
  each Reccmp-mapped function whose original and rebuilt code hold the same
  number of address references, a rebuilt reference into the tail with the
  original address at the same position (positional evidence, not proof);
- `second_pass_libc`: LIBC members linked after the tail, which only the tail
  needs (finding 658).

Usage: python3 tools/port_tail_inventory.py [--build build-reccmp] [--json OUT]
"""
from __future__ import annotations

import argparse
import bisect
import collections
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import identity_metrics  # noqa: E402
import link_order  # noqa: E402
import pe_audit  # noqa: E402
from translation_units import ROOT, read_rows  # noqa: E402

RANGES = ROOT / 'docs/coverage/translation-unit-ranges.csv'
TAIL_KINDS = ('tail', 'legacy')
DATA_SECTIONS = ('.rdata', '.data', '.bss')
# Symbols LIBC's start-up code references in the program (wincrt0.obj).
CRT_ENTRY_SYMBOLS = ('_WinMain@16', '_main', '_wWinMain@16')


class Lookup:
    """Interval lookup over (start, end, ...) tuples."""

    def __init__(self, items):
        self.items = sorted(items)
        self.starts = [x[0] for x in self.items]

    def __call__(self, address):
        i = bisect.bisect_right(self.starts, address) - 1
        if i >= 0 and address < self.items[i][1]:
            return self.items[i]
        return None


def member(obj):
    """Object basename of a map Lib:Object entry, lower case."""
    return obj.split(':', 1)[-1].lower()


def coff_data(path, owner):
    """Data symbol sizes and section bytes of one object, from its COFF sections.

    `owner` maps symbol name -> map object basename. Returns ({name: size},
    Counter of section bytes). A named data symbol owns the bytes up to the
    next named symbol of its section or the section's end. Non-COMDAT data
    sections always count; a COMDAT only when the map attributes its symbol
    to this object (LINK keeps one copy of a duplicated string literal).
    """
    import msvc_coff
    obj = msvc_coff.read_coff(path)
    name = Path(path).name.lower()
    by_section = collections.defaultdict(list)
    for sym in obj.symbols.values():
        if sym.section > 0 and sym.storage_class in (2, 3) and not sym.aux:
            sect = obj.section(sym.section)
            if not sect.is_code and not sect.is_debug:
                by_section[sym.section].append((sym.value, sym.name))
    sizes, sections = {}, collections.Counter()
    for sect in obj.sections:
        if sect.is_code or sect.is_debug:
            continue
        items = sorted(by_section.get(sect.index, []))
        if sect.comdat_selection is not None and not any(owner.get(n) == name for _, n in items):
            continue
        sections['.bss' if sect.is_bss else sect.name.split('$')[0]] += sect.size
        for i, (offset, sym) in enumerate(items):
            sizes[sym] = (items[i + 1][0] if i + 1 < len(items) else sect.size) - offset
    return sizes, sections


def rebuilt_symbols(pe, map_text, kinds, data_sizes=None):
    """Map symbols as (address, end, object, name, kind, section), by address.

    Section numbers follow the PE section table; `.data` symbols at or above
    the `.bss` group start count as `.bss`. A symbol owns the bytes up to the
    next one; `data_sizes` ({object basename: {name: size}}) caps a data
    symbol at its size in its object, so unnamed data of a later object
    (LIBC's constants) is not attributed to it.
    """
    groups, symbols = pe_audit.parse_map(map_text)
    base = pe.OPTIONAL_HEADER.ImageBase
    sections = {i + 1: s for i, s in enumerate(pe.sections)}
    ends, bss = {}, None
    for g in groups:
        ends[g['section']] = max(ends.get(g['section'], 0), g['offset'] + g['length'])
        if g['name'] == '.bss':
            bss = (g['section'], g['offset'])
    by_section = collections.defaultdict(list)
    for s in symbols:
        by_section[s['section']].append(s)
    out = []
    for number, items in by_section.items():
        if number not in sections:
            continue
        items.sort(key=lambda s: s['offset'])
        name = sections[number].Name.rstrip(b'\0').decode('ascii')
        va = base + sections[number].VirtualAddress
        for i, s in enumerate(items):
            end = items[i + 1]['offset'] if i + 1 < len(items) else ends.get(number, s['offset'])
            if end <= s['offset']:
                continue
            section = '.bss' if bss and number == bss[0] and s['offset'] >= bss[1] else name
            if section != '.text' and data_sizes:
                cap = data_sizes.get(member(s['object']), {}).get(s['name'])
                if cap is not None:
                    end = min(end, s['offset'] + cap)
            out.append((va + s['offset'], va + end, s['object'], s['name'],
                        pe_audit.classify_object(s['object'], kinds), section))
    out.sort()
    return out


def references(read, start, size, image):
    """Address references in order: direct call/jump targets outside the
    function and image-address operands."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    out = []
    for ins in md.disasm(read(start, size), start):
        branch = ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP)
        for op in ins.operands:
            if op.type == X86_OP_IMM:
                v = op.imm & 0xFFFFFFFF
                if branch and start <= v < start + size:
                    continue
                if branch or image[0] <= v < image[1]:
                    out.append(v)
            elif op.type == X86_OP_MEM and image[0] <= (op.mem.disp & 0xFFFFFFFF) < image[1]:
                out.append(op.mem.disp & 0xFFFFFFFF)
    return out


def data_references(read, start, size, image):
    """Aligned dwords in a data symbol that hold image addresses."""
    data = read(start, size)
    first = (-start) % 4
    return [v for v in (int.from_bytes(data[i:i + 4], 'little') for i in range(first, len(data) - 3, 4))
            if image[0] <= v < image[1]]


def unit_lookup(rows):
    """Original address -> unit id, from the text and data evidence ranges."""
    spans = []
    for r in rows:
        spans.append((int(r['text_start'], 16), int(r['text_end'], 16), r['unit_id']))
        for key in ('rdata_range', 'data_range', 'string_range', 'bss_range'):
            if r[key]:
                lo, hi = (int(x, 16) for x in r[key].split('-'))
                spans.append((lo, hi + 4, r['unit_id']))
    find = Lookup(spans)
    return lambda a: (find(a) or (None, None, None))[2]


def text_runs(symbols):
    """Consecutive `.text` symbols of one kind, as [kind, start, end, symbols]."""
    runs = []
    for va, end, obj, name, kind, section in symbols:
        if section != '.text':
            continue
        if runs and runs[-1][0] == kind:
            runs[-1][2] = end
            runs[-1][3] += 1
        else:
            runs.append([kind, va, end, 1])
    return runs


def unit_spans(symbols, units, rows):
    """Rebuilt `.text` span of each unit object against its original range."""
    firsts = {}
    for va, end, obj, name, kind, section in symbols:
        if section == '.text':
            firsts.setdefault(obj, va)
    order = sorted(firsts.items(), key=lambda x: x[1])
    text_end = max(s[1] for s in symbols if s[5] == '.text')
    spans = {}
    for i, (obj, start) in enumerate(order):
        unit = units.get(member(obj))
        if unit:
            spans[unit] = (start, order[i + 1][1] if i + 1 < len(order) else text_end)
    out = []
    for r in rows:
        if r['unit_id'] not in spans:
            continue
        start, end = spans[r['unit_id']]
        lo, hi = int(r['text_start'], 16), int(r['text_end'], 16)
        out.append({'unit_id': r['unit_id'], 'unit_name': r['unit_name'],
                    'original_start': hex(lo), 'rebuilt_start': hex(start),
                    'original_bytes': hi - lo, 'rebuilt_bytes': end - start, 'delta': (end - start) - (hi - lo)})
    return out


def build_objects(build):
    """Every object the matching build links, as {path: library}.

    Library is the archive the object comes from (`link-members.txt`:
    `lrr_tail`, `unit_<stem>`), `late_<stem>` for the late units, or None for
    an explicit object.
    """
    members = {}
    path = Path(build) / link_order.MEMBERS_FILE
    if path.is_file():
        for line in path.read_text(encoding='utf-8').splitlines():
            fields = line.split(None, 2)
            if len(fields) == 3:
                members[Path(fields[2]).resolve()] = fields[0]
    out = {}
    for obj in sorted((Path(build) / 'CMakeFiles').glob('*/**/*.obj')):
        top = obj.relative_to(Path(build) / 'CMakeFiles').parts[0]
        if top.startswith('link_'):
            out[obj] = members.get(obj.resolve())
        elif top.startswith('late_'):
            out[obj] = top[:-len('.dir')]
    return out


def reachability(objects, referrer_of, read=None):
    """What LINK /OPT:REF keeps of the tail library, from COFF relocations.

    `objects` is {path: library}; objects of `lrr_tail` are the tail, the
    others are linked whole and are the roots (their sections reference the
    tail through relocations). LINK resolves archive members first: a tail
    object is pulled when any section of a pulled object or a root
    references one of its external symbols, whether or not /OPT:REF later
    keeps that section. /OPT:REF then keeps the roots' references, every
    non-COMDAT section of a pulled object (data, `.CRT$XCU` initialisers) and
    what those reference; a COMDAT (each `/Gy` function, each string
    literal) stays only when a kept section references it.
    LIBC's reference to the entry (`WinMain`) is a root too. Returns the
    tail's kept code bytes and, for each referrer
    (`referrer_of(path)`), the code bytes kept only through its references.
    """
    if read is None:
        import msvc_coff
        read = msvc_coff.read_coff
    coff = {p: read(p) for p in objects}
    tail = {p for p, lib in objects.items() if lib == link_order.TAIL_LIBRARY}
    defs = {}
    for p, obj in coff.items():
        for sym in obj.symbols.values():
            if sym.section > 0 and sym.storage_class == 2 and not obj.section(sym.section).is_debug:
                key = (p, sym.section)
                if sym.name not in defs or p not in tail:
                    defs[sym.name] = key    # a duplicate COMDAT resolves to a root's copy

    def targets(p, sect):
        obj = coff[p]
        for r in sect.relocations:
            sym = obj.symbols.get(r.symbol)
            if sym is None:
                continue
            if sym.section > 0:
                yield (p, sym.section)
            elif sym.section == 0 and sym.name in defs:
                yield defs[sym.name]

    edges, code = {}, {}
    for p in tail:
        for sect in coff[p].sections:
            if sect.is_debug:
                continue
            edges[(p, sect.index)] = {t for t in targets(p, sect) if t[0] in tail}
            if sect.is_code:
                code[(p, sect.index)] = sect.size
    roots = collections.defaultdict(set)
    # LIBC's start-up code calls the program's entry (wincrt0.obj); LIBC.LIB
    # itself is not among the objects read here.
    roots['LIBC'].update(defs[n] for n in CRT_ENTRY_SYMBOLS if n in defs and defs[n][0] in tail)
    for p in coff:
        if p in tail:
            continue
        for sect in coff[p].sections:
            if not sect.is_debug:
                roots[referrer_of(p)].update(t for t in targets(p, sect) if t[0] in tail)

    def kept(start, exclude=frozenset()):
        # LINK resolves archive members before /OPT:REF: a pulled object's
        # every section, kept or not, pulls the members it references.
        pulled, work = set(), [n[0] for n in start]
        while work:
            p = work.pop()
            if p in pulled:
                continue
            pulled.add(p)
            for sect in coff[p].sections:
                work.extend(t[0] for t in edges.get((p, sect.index), ()))
        # /OPT:REF then keeps what the roots and the pulled objects'
        # non-COMDAT sections reference.
        keep = set()
        work = list(start) + [(p, sect.index) for p in pulled for sect in coff[p].sections
                              if not sect.is_debug and sect.comdat_selection is None
                              and (p, sect.index) not in exclude]
        while work:
            node = work.pop()
            if node in keep:
                continue
            keep.add(node)
            work.extend(edges.get(node, ()))
        return keep

    everything = set().union(*roots.values()) if roots else set()
    base = kept(everything)
    total = sum(code.get(n, 0) for n in base)
    by_object = collections.Counter()
    for n in base:
        by_object[n[0].name.lower()] += code.get(n, 0)
    out = {'kept_code_bytes': total, 'compiled_code_bytes': sum(code.values()),
           'kept_by_object': dict(by_object.most_common()), 'by_referrer': {}}
    for name, start in roots.items():
        if not start:
            continue
        rest = set().union(*(r for n, r in roots.items() if n != name))
        left = sum(code.get(n, 0) for n in kept(rest))
        out['by_referrer'][name] = {'referenced_sections': len(start), 'code_bytes_only_through': total - left}
    out['by_referrer'] = dict(sorted(out['by_referrer'].items(), key=lambda x: -x[1]['code_bytes_only_through']))
    # Greedy order over the tail objects the roots reference: retarget,
    # one object at a time, every root reference into the object whose
    # references keep the most tail code given the ones already retargeted.
    by_target = collections.defaultdict(set)
    for refs in roots.values():
        for node in refs:
            by_target[node[0].name.lower()].add(node)
    referrers = collections.defaultdict(set)
    for name, refs in roots.items():
        for node in refs:
            referrers[node[0].name.lower()].add(name)
    # Anchors: the pulled objects' non-COMDAT data sections (hook tables,
    # function tables, `.CRT$XCU`) that reference tail code. /OPT:REF keeps
    # them whole, so they keep what they point at.
    anchors = [(p, sect.index) for p in tail for sect in coff[p].sections
               if not sect.is_debug and not sect.is_code and sect.comdat_selection is None
               and (p, sect.index) in base and any(code.get(t) for t in edges.get((p, sect.index), ()))]
    every = set().union(*roots.values()) if roots else set()
    out['anchors'] = []
    for node in anchors:
        dropped = total - sum(code.get(n, 0) for n in kept(every, {node}))
        if dropped:
            sect = coff[node[0]].section(node[1])
            names = [s.name for s in coff[node[0]].symbols.values()
                     if s.section == node[1] and not s.name.startswith('.') and not s.aux]
            out['anchors'].append({'object': node[0].name.lower(), 'section': sect.name,
                                   'symbols': names[:3], 'code_bytes_only_through': dropped})
    out['anchors'].sort(key=lambda a: -a['code_bytes_only_through'])
    out['code_bytes_without_anchors'] = sum(code.get(n, 0) for n in kept(every, set(anchors)))
    out['anchor_sections'] = len(anchors)
    out['root_objects'] = {name: {'referenced_sections': len(refs), 'referrers': sorted(referrers[name]),
                                  'code_bytes_kept_alone': sum(code.get(n, 0) for n in kept(refs))}
                           for name, refs in sorted(by_target.items())}
    def greedy(exclude, start_total):
        left, order, current = dict(by_target), [], start_total
        while left and current:
            best = None
            for name in left:
                rest = set().union(*(r for n, r in left.items() if n != name))
                size = sum(code.get(n, 0) for n in kept(rest, exclude))
                if best is None or size < best[1]:
                    best = (name, size)
            order.append({'object': best[0], 'referenced_sections': len(left.pop(best[0])),
                          'referrers': sorted(referrers[best[0]]),
                          'code_bytes_dropped': current - best[1], 'code_bytes_left': best[1]})
            current = best[1]
        return order

    order = greedy(frozenset(), total)
    out['greedy_order_without_anchors'] = greedy(frozenset(anchors), out['code_bytes_without_anchors'])
    out['greedy_order'] = order
    return out


def inventory(original, rebuilt, map_text, entries, entities, rows, link_entries, object_paths, linked=None):
    kinds = pe_audit.object_kinds(link_entries)
    units = {Path(e.source).name.lower() + '.obj': e.key for e in link_entries
             if e.kind == 'unit' and e.applies_to('winapi')}
    sources = {Path(e.source).name.lower() + '.obj': e.source for e in link_entries if e.applies_to('winapi')}
    _, map_symbols = pe_audit.parse_map(map_text)
    owner = {s['name']: member(s['object']) for s in map_symbols}
    coff = {name: coff_data(path, owner) for name, path in object_paths.items()}
    symbols = rebuilt_symbols(rebuilt, map_text, kinds, {n: c[0] for n, c in coff.items()})
    symbol_at = Lookup(symbols)
    unit_of = unit_lookup(rows)
    base = rebuilt.OPTIONAL_HEADER.ImageBase
    image = rebuilt.get_memory_mapped_image()

    def rebuilt_read(a, n):
        return bytes(image[a - base:a - base + n])
    obase = original.pe.OPTIONAL_HEADER.ImageBase
    oimage = (obase, obase + original.pe.OPTIONAL_HEADER.SizeOfImage)
    rimage = (base, base + rebuilt.OPTIONAL_HEADER.SizeOfImage)

    def referrer(obj, kind):
        if kind == 'unit' and member(obj) in units:
            return units[member(obj)]
        return 'LIBC' if kind == 'libc' else member(obj)

    objects = collections.OrderedDict()
    for va, end, obj, name, kind, section in symbols:
        if kind not in TAIL_KINDS:
            continue
        o = objects.setdefault(obj, {'object': member(obj), 'source': sources.get(member(obj)), 'kind': kind,
                                     'bytes': collections.Counter(), 'functions': 0,
                                     'referenced_by': collections.Counter(),
                                     'referenced_by_tail': collections.Counter(),
                                     'references_units': collections.Counter(),
                                     'references_libc': collections.Counter(),
                                     'stands_in_for': collections.Counter()})
        if section == '.text' or member(obj) not in coff:
            o['bytes'][section] += end - va
        o['functions'] += section == '.text'
    for o in objects.values():
        if o['object'] in coff:
            o['bytes'].update({k: v for k, v in coff[o['object']][1].items() if k in DATA_SECTIONS})

    for va, end, obj, name, kind, section in symbols:
        if kind.startswith('library:') or kind == 'import':
            continue    # DirectInput's data formats and GUIDs; import thunks
        if section == '.text':
            refs = references(rebuilt_read, va, end - va, rimage)
        elif section in ('.rdata', '.data'):
            refs = data_references(rebuilt_read, va, end - va, rimage)
        else:
            continue
        for ref in refs:
            sym = symbol_at(ref)
            if not sym or sym[2] == obj or (section != '.text' and ref != sym[0]):
                continue    # a data dword counts only as a pointer to a symbol's start
            if kind in TAIL_KINDS:
                if sym[4] in TAIL_KINDS:
                    objects[sym[2]]['referenced_by_tail'][member(obj)] += 1
                elif sym[4] == 'unit':
                    objects[obj]['references_units'][referrer(sym[2], 'unit')] += 1
                elif sym[4] == 'libc':
                    objects[obj]['references_libc'][member(sym[2])] += 1
            elif sym[4] in TAIL_KINDS:
                objects[sym[2]]['referenced_by'][referrer(obj, kind)] += 1

    sizes = identity_metrics.function_sizes(
        entries, entities,
        (original.read, {int(e['address'], 16) for e in entries}, original.text[1]),
        (rebuilt_read, {s[0] for s in symbols if s[5] == '.text'}, max(s[1] for s in symbols if s[5] == '.text')))
    pairs = {e['orig']: e['recomp'] for e in entities if e.get('type') == 1 and e.get('orig') and e.get('recomp')}
    paired = 0
    for address, (osize, rsize) in sizes.items():
        new = references(rebuilt_read, pairs[address], rsize, rimage)
        old = references(original.read, address, osize, oimage)
        if len(new) != len(old):
            continue
        paired += 1
        for ref, was in zip(new, old):
            sym = symbol_at(ref)
            if sym and sym[4] in TAIL_KINDS:
                objects[sym[2]]['stands_in_for'][f'{sym[3]} -> {was:#x} {unit_of(was) or "unattributed"}'] += 1

    runs = text_runs(symbols)
    entry = base + rebuilt.OPTIONAL_HEADER.AddressOfEntryPoint
    oentry = obase + original.pe.OPTIONAL_HEADER.AddressOfEntryPoint
    crt_start = min(int(r['text_start'], 16) for r in rows if r['unit_kind'] == 'crt')
    first_libc = next(r for r in runs if r[0] == 'libc')
    last_tail = max(i for i, r in enumerate(runs) if r[0] in TAIL_KINDS)
    second_pass = [(s[3], member(s[2]), s[1] - s[0]) for s in symbols
                   if s[5] == '.text' and s[4] == 'libc' and s[0] >= runs[last_tail][2]]
    result = {
        'mapped_functions': len(sizes), 'paired_functions': paired,
        'layout': [{'kind': k, 'start': hex(a), 'end': hex(b), 'bytes': b - a, 'symbols': n} for k, a, b, n in runs],
        'entry_point': {'original': hex(oentry), 'rebuilt': hex(entry), 'delta': entry - oentry,
                        'unit_segment_end': {'original': hex(crt_start), 'rebuilt': hex(first_libc[1]),
                                             'delta': first_libc[1] - crt_start},
                        'libc_before_entry': {'original': oentry - crt_start, 'rebuilt': entry - first_libc[1],
                                              'delta': (entry - first_libc[1]) - (oentry - crt_start)}},
        'unit_spans': unit_spans(symbols, units, rows),
        'second_pass_libc': {'functions': len(second_pass), 'bytes': sum(x[2] for x in second_pass),
                             'members': sorted({x[1] for x in second_pass})},
        'objects': [],
    }
    if linked:
        by_source = {}
        for e in link_entries:
            if e.applies_to('winapi'):
                by_source[e.source] = e.key if e.kind == 'unit' else e.source
        def referrer_of(path):
            text = path.as_posix()
            rel = text.split('/src/', 1)[1][:-len('.obj')] if '/src/' in text else path.name
            return by_source.get(rel, rel)
        result['removal'] = reachability(linked, referrer_of)
    for o in objects.values():
        result['objects'].append({**o, 'bytes': dict(o['bytes']),
                                  **{k: dict(o[k].most_common()) for k in
                                     ('referenced_by', 'referenced_by_tail', 'references_units',
                                      'references_libc', 'stands_in_for')}})
    result['objects'].sort(key=lambda o: (-sum(o['bytes'].values()), o['object']))
    totals = collections.Counter()
    for o in result['objects']:
        totals.update(o['bytes'])
    result['totals'] = dict(totals)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--build', type=Path, default=ROOT / 'build-reccmp')
    parser.add_argument('--original', type=Path, default=ROOT / 'reference/LegoRR.exe')
    parser.add_argument('--json', type=Path, help='write the full inventory here')
    args = parser.parse_args()
    import pefile
    from translation_units import Binary
    report = args.build / 'reccmp-report'
    paths = {Path(p).name.lower(): Path(p) for p in link_order.member_objects(args.build).values()}
    result = inventory(Binary(args.original), pefile.PE(str(args.build / 'LegoRR.exe')),
                       (args.build / 'LegoRR.map').read_text(errors='replace'),
                       json.loads((report / 'results.json').read_text())['data'],
                       json.loads((report / 'entities.json').read_text()),
                       read_rows(RANGES), link_order.load(),
                       {k: p for k, p in paths.items() if p.is_file()}, build_objects(args.build))
    if args.json:
        args.json.write_text(json.dumps(result, indent=1) + '\n')
    e = result['entry_point']
    print(f"entry point {e['rebuilt']} vs {e['original']} ({e['delta']:+}): unit segment end "
          f"{e['unit_segment_end']['delta']:+}, LIBC before the entry {e['libc_before_entry']['delta']:+}")
    for r in result['layout']:
        print(f"  {r['kind']:16} {r['start']}-{r['end']} {r['bytes']:8,} bytes {r['symbols']:5} symbols")
    r = result['removal']
    print(f"tail code kept (COFF /OPT:REF model): {r['kept_code_bytes']:,} of {r['compiled_code_bytes']:,} compiled")
    print(f"  without any of the {r['anchor_sections']} anchor sections: {r['code_bytes_without_anchors']:,}")
    for a in r['anchors'][:25]:
        print(f"  anchor {a['object']:32} {a['section']:8} {','.join(a['symbols'])[:60]:60} {a['code_bytes_only_through']:7,}")
    for name, v in sorted(r['root_objects'].items(), key=lambda x: -x[1]['code_bytes_kept_alone']):
        print(f"  {name:32} alone keeps {v['code_bytes_kept_alone']:7,} ({v['referenced_sections']} refs from {','.join(v['referrers'][:6])})")
    print('  with the anchors gone:')
    for v in r['greedy_order_without_anchors']:
        print(f"  retarget {v['object']:32} {v['referenced_sections']:4} refs: -{v['code_bytes_dropped']:7,} -> "
              f"{v['code_bytes_left']:7,} left  ({','.join(v['referrers'][:5])})")
    print('  as linked:')
    for v in r['greedy_order']:
        print(f"  retarget {v['object']:32} {v['referenced_sections']:4} refs: -{v['code_bytes_dropped']:7,} -> "
              f"{v['code_bytes_left']:7,} left  ({','.join(v['referrers'][:5])})")
    s = result['second_pass_libc']
    print(f"LIBC after the tail: {s['functions']} functions, {s['bytes']:,} bytes")
    print(f"{'object':36} {'.text':>7} {'.rdata':>6} {'.data':>6} {'.bss':>7}  referenced by / references units")
    for o in result['objects']:
        b = o['bytes']
        refs = ','.join(list(o['referenced_by'])[:3]) or '-'
        uses = ','.join(list(o['references_units'])[:3]) or '-'
        print(f"{o['object']:36} {b.get('.text', 0):7} {b.get('.rdata', 0):6} {b.get('.data', 0):6} "
              f"{b.get('.bss', 0):7}  {refs} / {uses}")
    t = result['totals']
    print(f"{'total':36} {t.get('.text', 0):7} {t.get('.rdata', 0):6} {t.get('.data', 0):6} {t.get('.bss', 0):7}")


if __name__ == '__main__':
    main()
