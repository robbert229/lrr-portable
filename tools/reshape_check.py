#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Compare a translation unit's own preprocessed tokens and matching object."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import struct
import sys
import tempfile

import match_function as mf
import msvc_coff

ROOT = Path(__file__).resolve().parent.parent
REFERENCE_FLAGS = ['-U__x86_64__', '-U__linux__', '-U__unix__', '-D__i386__',
                   '-D_X86_', '-D_WIN32', '-D__MINGW32__', '-DWIN32',
                   '-DLEGORRR_BACKEND_WINAPI=1']
MARKER = re.compile(r'^\s*#\s*(?:line\s+)?\d+\s+"((?:[^"\\]|\\.)*)"')
# Preprocessing tokens, preserving literals and distinguishing adjacent operators.
TOKEN = re.compile(r'''(?:u8|u|U|L)?R"([^ ()\\\t\r\n]*)\(.*?\)\1"|(?:u8|u|U|L)?"(?:\\.|[^"\\])*"|(?:u|U|L)?'(?:\\.|[^'\\])*'|[a-zA-Z_$][\w$]*|(?:\d|\.\d)(?:[eEpP][+-]|[\w.])*|(?:%:%:|>>=|<<=|\.\.\.|->\*|##|::|\.\*|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||\*=|/=|%=|\+=|-=|&=|\^=|\|=|<:|:>|<%|%>|%:)|[^\s]''', re.S)


def tokens_from_file(output: str, source: Path) -> list[str]:
    target = os.path.normcase(os.path.realpath(source))
    active = False
    lines = []
    for line in output.splitlines(keepends=True):
        marker = MARKER.match(line)
        if marker:
            name = marker.group(1).replace('\\\\', '\\')
            if re.match(r'(?i)^z:[\\/]', name):
                name = name[2:].replace('\\', '/')
            active = os.path.normcase(os.path.realpath(name)) == target
        elif active:
            lines.append(line)
    if not lines:
        raise RuntimeError(f'no preprocessed lines attributed to {source}')
    return [m.group(0) for m in TOKEN.finditer(''.join(lines))]


def strip_includes(blob):
    """Remove include directives while retaining line numbers and compare them separately."""
    # Join continuation lines into each directive, then retain the newlines.
    pattern = re.compile(rb'(?m)^[ \t]*\#[ \t]*include\b(?:[^\r\n]*\\\r?\n)*[^\r\n]*(?:\r?\n|$)')
    includes = pattern.findall(blob)
    return pattern.sub(lambda m: b'\n' * m.group().count(b'\n'), blob), includes


def preprocess_reference(candidate):
    compiler = shutil.which('g++')
    if not compiler:
        raise RuntimeError('host g++ not found for emulated MinGW preprocessing')
    return run([compiler, '-E', '-x', 'c++', *REFERENCE_FLAGS, str(candidate)], ROOT)


def rename_identifier(text: str, renames: dict[str, str]) -> str:
    """Spell each renamed identifier in a COFF symbol name as its new name.

    The identifier matches whole within MSVC's decorations: plain ``old``,
    C ``_old`` and a file static's ``_old$S12``, and C++ ``?old@@3HA`` or a
    scope ``?f@old@@``. ``old_more`` and ``_old_more$S1`` are not renamed."""
    for old, new in renames.items():
        text = re.sub(r'(?:(?<=^)|(?<=^_)|(?<=^\?)|(?<=@))' + re.escape(old) + r'(?=$|\$|@)', new, text)
    return text


def compare_objects(left: bytes, right: bytes, renames: dict[str, str] | None = None) -> list[str]:
    """Compare non-debug contents; ``renames`` maps names in ``left`` first."""
    a, b = (msvc_coff.parse_coff(x) for x in (left, right))
    renames = renames or {}
    errors = []
    def sections(obj):
        return [s for s in obj.sections if not s.name.startswith('.debug$')]
    sa, sb = sections(a), sections(b)
    if [s.name for s in sa] != [s.name for s in sb]:
        errors.append('section names differ')
    for x, y in zip(sa, sb):
        if x.name != y.name:
            continue
        if (x.data, x.size, x.characteristics) != (y.data, y.size, y.characteristics):
            errors.append(f'section {x.name} bytes/size/flags differ')
        def relocs(obj, section):
            return [(r.offset, r.type, obj.symbols[r.symbol].name) for r in section.relocations]
        renamed = [(o, t, rename_identifier(n, renames)) for o, t, n in relocs(a, x)]
        if renamed != relocs(b, y):
            errors.append(f'section {x.name} relocations differ')
    def symbols(obj):
        result = []
        for s in obj.symbols.values():
            # File/debug records and symbols belonging to debug sections carry
            # source paths and line numbers, not linkable object contents.
            if s.section == -2 or (s.section > 0 and obj.section(s.section).name.startswith('.debug$')):
                continue
            section = obj.section(s.section).name if s.section > 0 else s.section
            result.append((s.name, section, s.value, s.storage_class))
        return result
    if [(rename_identifier(n, renames), *rest) for n, *rest in symbols(a)] != symbols(b):
        errors.append('symbol table differs')
    return errors


def run(argv, cwd):
    p = subprocess.run(argv, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode:
        raise RuntimeError(f"command failed ({p.returncode}): {shlex.join(argv)}\n{p.stderr.decode('latin1')}")
    return p.stdout.decode('latin1')


def database(build, extra):
    db = build / 'compile_commands.json'
    if not db.exists():
        run(['cmake', '-S', str(ROOT), '-B', str(build), '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', *extra], ROOT)
    cache = (build / 'CMakeCache.txt').read_text()
    expected = 'winapi' if build.name == 'reshape-winapi' else 'native'
    if not re.search(r'^PLATFORM:[^=]+=' + expected + r'$', cache, re.M):
        raise RuntimeError(f'{build} is not configured for {expected}; configure a separate build first')
    return json.loads(db.read_text())


def entry_path(entry, name):
    """Resolve a compile-database path, which may be relative to the entry's directory."""
    return (Path(entry['directory']) / name).resolve()


def entry_for(entries, source):
    for e in entries:
        if entry_path(e, e['file']) == source:
            return e
    raise RuntimeError(f'no compile command for {source}')


def preprocess_gcc(entry, source, candidate):
    args = entry.get('arguments') or shlex.split(entry['command'])
    result = []
    skip = False
    for arg in args:
        if skip:
            skip = False
            continue
        if arg in ('-o', '-MF', '-MT', '-MQ'):
            skip = True
        elif arg in ('-c', '-MD', '-MMD', '-MP'):
            continue
        elif not arg.startswith('-') and entry_path(entry, arg) == source:
            result.append(str(candidate))
        else:
            result.append(arg)
    result[1:1] = ['-E', '-I' + str(source.parent)]
    return run(result, Path(entry['directory']))


def preprocess_msvc(env, command, candidate, work):
    args = []
    for arg in command.argv:
        if Path(arg).resolve() == command.source:
            args.append(str(candidate))
        elif arg.lower() in ('/c', '-c') or arg.lower().startswith(('/fo', '-fo', '/fd', '-fd')):
            continue
        else:
            args.append(arg)
    args[1:1] = ['/E', '/I' + str(command.source.parent)]
    log = work / 'preprocess.log'
    rc = env.run(args, cwd=work, log_path=log, timeout=600)
    output = log.read_bytes().decode('latin1').replace('\r', '')
    if rc:
        raise RuntimeError(f'MSVC preprocessing failed ({rc}): {output}')
    return output


def revision(path, rev):
    if rev == 'worktree':
        return path.read_bytes()
    return subprocess.check_output(['git', 'show', f'{rev}:{path.relative_to(ROOT).as_posix()}'], cwd=ROOT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', nargs='+')
    parser.add_argument('--base', default='HEAD~1')
    parser.add_argument('--head', default='worktree')
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--rename', action='append', default=[], metavar='OLD=NEW',
                        help='compare modulo renaming identifier OLD (in --base) to NEW; repeatable')
    args = parser.parse_args()
    renames = {}
    for item in args.rename:
        old, sep, new = item.partition('=')
        if not sep or not re.fullmatch(r'[A-Za-z_$][\w$]*', old) or not re.fullmatch(r'[A-Za-z_$][\w$]*', new):
            parser.error(f'--rename expects OLD=NEW identifiers, got {item!r}')
        renames[old] = new
    # Use the developer environment for all compilers, including CMake's
    # native compiler, when Wine is not available on the host.
    if not shutil.which('wine') and shutil.which('distrobox') and not os.environ.get('RESHAPE_IN_BOX'):
        os.environ['RESHAPE_IN_BOX'] = '1'
        os.execvp('distrobox', ['distrobox', 'enter', os.environ.get('MATCH_FN_DISTROBOX', 'developer'), '--', 'python3', str(Path(__file__).resolve()), *sys.argv[1:]])
    results = []
    try:
        env = mf.Environment.detect(argparse.Namespace(build_dir=None))
        commands = mf.load_compile_commands(env, {})
        native = database(ROOT / 'build', ['-DPLATFORM=native'])
        mingw = None
        if shutil.which('i686-w64-mingw32-g++'):
            mingw = database(ROOT / 'build-match' / 'reshape-winapi', ['-DPLATFORM=winapi', '-DCMAKE_TOOLCHAIN_FILE=' + str(ROOT / 'cross/i686-toolchain.cmake')])
        for name in args.files:
            source = (ROOT / name).resolve()
            source.relative_to(ROOT)
            cmd = next((c for c in commands if c.source == source), None)
            if cmd is None:
                raise RuntimeError(f'no matching compile command for {source}')
            blobs = [revision(source, args.base), revision(source, args.head)]
            stripped = [strip_includes(blob) for blob in blobs]
            with tempfile.TemporaryDirectory(prefix='.reshape-', dir=ROOT) as tmp:
                work = Path(tmp)
                candidate = work / source.name
                streams = {k: [] for k in ('native', 'matching', 'mingw')}
                objects = []
                for blob, (preprocessed_source, _) in zip(blobs, stripped):
                    candidate.write_bytes(preprocessed_source)
                    streams['native'].append(tokens_from_file(preprocess_gcc(entry_for(native, source), source, candidate), candidate))
                    streams['matching'].append(tokens_from_file(preprocess_msvc(env, cmd, candidate, work), candidate))
                    if mingw is not None:
                        ref_output = preprocess_gcc(entry_for(mingw, source), source, candidate)
                    else:
                        ref_output = preprocess_reference(candidate)
                    streams['mingw'].append(tokens_from_file(ref_output, candidate))
                    candidate.write_bytes(blob)
                    obj, _ = mf.compile_source(env, cmd, work, candidate=candidate)
                    objects.append(obj.read_bytes())
                checks = {'includes': 'identical' if stripped[0][1] == stripped[1][1] else 'different include directives'}
                for key in streams:
                    streams[key][0] = [renames.get(t, t) for t in streams[key][0]]
                    if streams[key][0] == streams[key][1]:
                        checks[key] = 'identical'
                        if key == 'mingw':
                            checks[key] += ' (cross-compiler)' if mingw is not None else ' (emulated: host gcc + reference defines)'
                    else:
                        l, r = streams[key]
                        index = next((i for i, (x, y) in enumerate(zip(l, r)) if x != y), min(len(l), len(r)))
                        checks[key] = f'different at token {index}: {l[index:index+1]} -> {r[index:index+1]}'
                        if key == 'mingw':
                            checks[key] += ' (cross-compiler)' if mingw is not None else ' (emulated: host gcc + reference defines)'
                diffs = compare_objects(*objects, renames=renames)
                checks['object'] = '; '.join(diffs) if diffs else 'identical (including .drectve when present)'
                result = {'file': name, 'checks': checks}
                if renames:
                    result['renames'] = renames
                results.append(result)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError, mf.MatchError, msvc_coff.CoffError, struct.error, IndexError, KeyError) as exc:
        if args.json:
            print(json.dumps({'results': results, 'error': str(exc)}))
        else:
            print(f'error: {exc}', file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps({'results': results}, indent=2))
    else:
        for result in results:
            print(result['file'] + ':')
            for old, new in result.get('renames', {}).items():
                print(f'  rename: {old} -> {new} (base spelled as head)')
            for key, value in result['checks'].items():
                print(f'  {key}: {value}')
    return int(any(not v.startswith('identical') for r in results for v in r['checks'].values()))


if __name__ == '__main__':
    sys.exit(main())
