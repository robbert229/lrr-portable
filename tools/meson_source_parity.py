#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Compare native CMake and Meson project source lists without configuring either."""

import argparse
from collections import Counter
from pathlib import Path
import posixpath
import re
import sys

import link_order

ROOT = Path(__file__).resolve().parent.parent


def capture(pattern, text, label):
    matches = list(re.finditer(pattern, text, re.S))
    if len(matches) != 1:
        raise ValueError(f'{label}: expected exactly one recognizable declaration')
    return matches[0].group(1)


def literals(text):
    # Reject nonliteral list expressions instead of silently missing sources.
    text = re.sub(r'#.*', '', text)
    paths = re.findall(r"'([^']+)'", text)
    if re.sub(r"'[^']+'|[\s,]", '', text):
        raise ValueError('source list contains an unsupported expression')
    return paths


def meson_array(text, name):
    return literals(capture(r'\b' + name + r'\s*=\s*\[(.*?)\]', text, name))


def compare(root):
    root = Path(root)
    read = lambda p: (root / p).read_text(encoding='utf-8')
    cmake = read('CMakeLists.txt')
    meson = {p: read('src/' + p + '/meson.build') for p in ('game', 'gods98', 'platform')}
    src = read('src/meson.build')
    entries = link_order.load(root)
    errors = []
    def check(label, expected, actual):
        expected, actual = Counter(expected), Counter(actual)
        for path, count in sorted((expected - actual).items()):
            errors.append(f'{label}: missing from Meson: {path} ({count})')
        for path, count in sorted((actual - expected).items()):
            errors.append(f'{label}: extra in Meson: {path} ({count})')
    for lib in ('game', 'gods98'):
        text = meson[lib]
        # Verify that the checked lists are the ones used by the target.
        capture(r"static_library\('" + lib + r"',\s*(" + lib + r"_sources,\s*" + lib + r"_c_as_cpp,\s*" + ("building_scene_sources" if lib == 'game' else 'gods98_backend_sources') + r")\s*,", text, lib + ' target')
        cpp_sources = meson_array(text, lib + '_sources')
        c_sources = meson_array(text, lib + '_c_sources')
        if any(not p.endswith('.cpp') for p in cpp_sources) or any(not p.endswith('.c') for p in c_sources):
            raise ValueError(lib + ': sources must be in the correct C or C++ list')
        capture(r'(foreach source : ' + lib + r'_c_sources\s+' + lib + r"_c_as_cpp \+= configure_file\(input : source, output : '@PLAINNAME@.cpp', copy : true\)\s+endforeach)", text, lib + ' C++ copies')
        actual = [posixpath.normpath('src/' + lib + '/' + p) for p in cpp_sources + c_sources]
        check(lib, [e.path for e in entries if e.library == lib and e.applies_to('native')], actual)
    # Loop-created test executables must cover the same names and files.
    for prefix, label, text, directory in (
            ('GODS98', 'gods98 tests', src, 'gods98'),
            ('GAME', 'game tests', meson['game'], 'game')):
        names = capture(r'set\(' + prefix + r'_TESTS\s+(.*?)\)', cmake, prefix + ' tests').split()
        sources = capture(r'set\(' + prefix + r'_TEST_SOURCES\s+(.*?)\)', cmake, prefix + ' test sources').split()
        if len(names) != len(sources):
            raise ValueError(prefix + ': test names and sources have different lengths')
        pairs = re.findall(r"\['([^']+)',\s*'([^']+)'\]", text)
        # The gods98 loop includes crt separately; CMake declares it separately.
        pairs = [(name, path) for name, path in pairs if name != 'crt']
        expected = [name + ':src/' + directory + '/' + path for name, path in zip(names, sources)]
        actual = [name + ':src/' + (path if label == 'gods98 tests' else directory + '/' + path) for name, path in pairs]
        check(label, expected, actual)
    platform = meson['platform']
    glue = capture(r"elif platform == 'native'.*?platform_glue = static_library\('platform_glue',\s*(.*?),\s*include_directories", platform, 'platform glue')
    check('platform_glue', [e.path for e in entries if e.library == 'platform_glue' and e.applies_to('native')], ['src/platform/' + p for p in literals(glue)])
    entry = capture(r"elif platform == 'native'\s*platform_entry = files\((.*?)\)", platform, 'native entry')
    program = capture(r"executable\('LegoRR',\s*sources\s*:\s*\[(.*?)\]", src, 'program')
    if not re.search(r'platform_entry\s*,\s*extra_sources\s*$', program):
        raise ValueError('program: unsupported source expression')
    program = re.sub(r'platform_entry\s*,\s*extra_sources\s*$', '', program).rstrip(' ,\n')
    actual = ['src/' + p for p in literals(program)] + ['src/platform/' + p for p in literals(entry)]
    extra = capture(r'set\(LRR_SOURCES\s+\$\{LRR_LINK_SOURCES_program\}(.*?)\)', cmake, 'CMake program sources')
    expected_extra = ['src/game/' + p for p in re.findall(r'\$\{GAME_DIR\}/([^\s)]+)', extra)]
    check('program', [e.path for e in entries if e.library == 'program' and e.applies_to('native')] + expected_extra, actual)
    cmake_mini = capture(r'set\(MINIWIN_SOURCES\s+(.*?)\)', cmake, 'CMake miniwin')
    expected_mini = cmake_mini.split()
    meson_mini = capture(r"static_library\('miniwin',\s*(.*?)\s*include_directories", platform, 'Meson miniwin')
    actual_mini = re.findall(r"miniwin_src\s*/\s*'src/([^']+)'", meson_mini)
    if re.sub(r"miniwin_src\s*/\s*'src/[^']+'|[\s,]", '', meson_mini):
        raise ValueError('Meson miniwin: unsupported source expression')
    check('miniwin', expected_mini, actual_mini)
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    args = parser.parse_args()
    try:
        errors = compare(args.root)
    except (ValueError, OSError) as exc:
        print(exc, file=sys.stderr)
        return 1
    for error in errors:
        print(error, file=sys.stderr)
    if not errors:
        print('native CMake/Meson source parity: OK')
    return bool(errors)


if __name__ == '__main__':
    sys.exit(main())
