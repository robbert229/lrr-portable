#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Run from the developer distrobox: python3 tools/experiments/449/run.py.

Outputs sources, COFF objects, archives, executables and maps under build-449.
Requires the matching MSVC 5.0 Wine wrappers. No game data is used.
"""
from pathlib import Path
import os
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
from msvc_coff import read_coff

OUT = ROOT / 'build-449' / 'suite'
OUT.mkdir(parents=True, exist_ok=True)
TC = Path(os.environ.get('REPROBIT_MSVC_5_0_RTM_ROOT', Path.home() / '.local/share/reprobit/toolchains/msvc_5_0_rtm')) / 'wine/x86'

def run(*args):
    subprocess.run([str(args[0]), *args[1:]], cwd=OUT, check=True, stdout=subprocess.DEVNULL)

def build(name, source):
    (OUT / (name + '.c')).write_text(source)
    run(TC / 'cl', '/nologo', '/TC', '/ML', '/O2', '/Ob1', '/DNDEBUG', '/c', name + '.c')
    obj = read_coff(OUT / (name + '.obj'))
    return [(s.name, s.value) for s in obj.symbols.values() if s.is_external and s.section == 0]

def link(name, *objects):
    run(TC / 'link', '/nologo', '/INCREMENTAL:NO', '/MAP:' + name + '.map', '/ENTRY:mainCRTStartup', '/SUBSYSTEM:CONSOLE', '/NODEFAULTLIB', '/OUT:' + name + '.exe', *objects)
    text = (OUT / (name + '.map')).read_text(errors='replace')
    rows = re.findall(r'^\s*[0-9A-Fa-f]+:[0-9A-Fa-f]+\s+(\S+)\s+([0-9A-Fa-f]{8})\s+<common>', text, re.M)
    print(name, ' '.join('%s=%s' % (n, a) for n, a in rows))

sources = {
 'a': 'int zebra[3]; int alpha[2]; int shared[2]; int bee; int use_a(void) { return zebra[0]+alpha[0]+shared[0]+bee; }',
 'renamed': 'int zebra[3]; int aardvark[2]; int shared[2]; int bee; int use_a(void) { return zebra[0]+aardvark[0]+shared[0]+bee; }',
 'b': 'int delta[5]; int gamma[1]; int shared[7]; int cat; int use_b(void) { return delta[0]+gamma[0]+shared[0]+cat; }',
 'm': 'extern int use_a(void),use_b(void); void mainCRTStartup(void) { use_a(); use_b(); }',
 'x': 'extern int zebra[], delta[], shared[]; int use_x(void) { return zebra[0]+delta[0]+shared[0]; }',
 'sizes': 'char s01[1],s02[2],s03[3],s04[4],s05[5],s07[7],s08[8],s09[9],s15[15],s16[16],s17[17],s31[31],s32[32],s33[33],s64[64],s65[65]; int use_sizes(void) { return s01[0]+s02[0]+s03[0]+s04[0]+s05[0]+s07[0]+s08[0]+s09[0]+s15[0]+s16[0]+s17[0]+s31[0]+s32[0]+s33[0]+s64[0]+s65[0]; }',
 'align': '#pragma pack(1)\nstruct packed { char c; double d; };\n#pragma pack()\nstruct natural { char c; double d; };\nstruct packed packval; struct natural natval; char tiny; char bigarr[129]; int use_align(void) { return packval.c+natval.c+tiny+bigarr[0]; }',
 'ma': 'extern int use_align(void); void mainCRTStartup(void) { use_align(); }',
 'ms': 'extern int use_sizes(void); void mainCRTStartup(void) { use_sizes(); }',
 'libmain': 'extern int use_a(void),use_b(void); void mainCRTStartup(void) { use_b(); use_a(); }',
}
for n, src in sources.items():
    print(n, 'COFF', build(n, src))
run(TC / 'lib', '/nologo', '/OUT:ab.lib', 'a.obj', 'b.obj')
for n, objs in [('ab', ('m.obj','a.obj','b.obj')), ('ba', ('m.obj','b.obj','a.obj')), ('rename', ('m.obj','renamed.obj','b.obj')), ('ref_first', ('x.obj','m.obj','a.obj','b.obj')), ('ref_last', ('m.obj','a.obj','b.obj','x.obj')), ('archive', ('libmain.obj','ab.lib')), ('archive_first', ('ab.lib','libmain.obj')), ('size', ('ms.obj','sizes.obj')), ('align', ('ma.obj','align.obj'))]:
    link(n, *objs)
