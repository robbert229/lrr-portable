#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Reject diagnostics from matching-build and Reccmp command output."""
import re
import sys

# The original returns a pointer to a stack result; preserve its shape until
# its unit is reconstructed (finding 379). MSVC 5 warns about that ABI. The
# function, com_property_result_get (the port's model of 0x00477310), was
# relocated with G98-003 (Viewports.c) from view.cpp into viewports.cpp, and
# moved unannotated to the port tail viewports_port.cpp when the unit was
# rebuilt as viewports.c (finding 593).
ALLOW = [re.compile(r'[/\\]src[/\\]gods98[/\\]viewports_port\.cpp\(\d+\) : warning C4172: returning address of local variable or temporary$', re.I),
         # Lego_MainLoop (0x004232e0, GAME-021 lego_unit.c) passes two locals
         # it never sets to the crystal panel when there is no level: the
         # original reads the same uninitialised stack slots (issue #544).
         re.compile(r'[/\\]src[/\\]game[/\\]lego_unit\.c\(\d+\) : warning C4700: local variable \'(used|crystals)\' used without having been initialized$', re.I),
         # The units the original compiled without optimisation take /Od
         # after the configuration's /O2 (LRR_UNOPTIMISED_UNITS in
         # CMakeLists.txt; the camera unit, finding 587).
         re.compile(r"^Command line warning D4025 : overriding '/O2' with '/Od'$"),
         # This toolchain omits the optional resource-compiler message catalog.
         re.compile(r'^WARNING:  missing cvtres\.err; displaying error numbers without messages\.$')]
DIAGNOSTIC = re.compile(r'\[(?:ERROR|WARNING|CRITICAL)\]|\bwarning\s+(?:[A-Z]+\d+\s*:|:)|^\s*(?:warning|error|fatal error)\s*:', re.I)

# Reccmp colours its log level ("[\x1b[31mERROR\x1b[0m]"), which hid its
# diagnostics from DIAGNOSTIC until the colour codes were removed (finding 569).
ANSI = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')

def unexpected(lines):
    plain = (ANSI.sub('', line).rstrip('\r\n') for line in lines)
    return [line for line in plain
            if DIAGNOSTIC.search(line) and not any(p.search(line) for p in ALLOW)]

def main():
    with open(sys.argv[1], encoding='utf-8', errors='replace') as f:
        failures = unexpected(f)
    if failures:
        print('Unexpected matching-build diagnostics:', file=sys.stderr)
        print('\n'.join(failures), file=sys.stderr)
        return 1
    return 0

if __name__ == '__main__':
    sys.exit(main())
