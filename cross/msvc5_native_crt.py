#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Stage MSVC 5.0's LINK.EXE so that it runs on the toolchain's own MSVCRT.

LINK 5.0 orders each DLL's import address table with the C runtime's
``qsort`` (finding 694). Under Wine, ``MSVCRT.dll`` resolves to Wine's
builtin, whose ``qsort`` permutes equal keys differently from Microsoft's
5.00 runtime, so the IAT order then differs from a link on Windows. Wine
itself needs its builtin ``msvcrt``, so the native one cannot be forced by a
DLL override.

This script copies ``link.exe`` and the DLLs it loads that import the C
runtime into STAGE, renames the toolchain's ``msvcrt.dll`` to
``MSVCR5.dll`` there and points their imports at that name (the same
length, so only the import name's bytes change). Nothing is written to the
toolchain itself. The copies are rebuilt when a source file is newer.

usage: msvc5_native_crt.py TOOLCHAIN_BIN STAGE
"""

import os
import shutil
import sys

CRT_NAME = b"MSVCRT.dll"
STAGED_CRT_NAME = b"MSVCR5.dll"
# link.exe and the DLLs it loads from the toolchain, each importing MSVCRT.
PATCHED = ("link.exe", "mspdb50.dll", "msdis100.dll", "msvcp50.dll")


def find_file(directory, name):
    for entry in os.listdir(directory):
        if entry.lower() == name.lower():
            return os.path.join(directory, entry)
    raise FileNotFoundError(os.path.join(directory, name))


def patch_import_name(data):
    """Replace the image's import of MSVCRT.dll (any case); return the count."""
    lowered = data.lower()
    needle = CRT_NAME.lower() + b"\0"
    count = 0
    start = 0
    while True:
        index = lowered.find(needle, start)
        if index < 0:
            return count
        data[index:index + len(STAGED_CRT_NAME)] = STAGED_CRT_NAME
        count += 1
        start = index + len(needle)


def stage(bin_dir, stage_dir):
    os.makedirs(stage_dir, exist_ok=True)
    jobs = [(find_file(bin_dir, "msvcrt.dll"), STAGED_CRT_NAME.decode(), False)]
    jobs += [(find_file(bin_dir, name), name, True) for name in PATCHED]
    for source, name, patch in jobs:
        target = os.path.join(stage_dir, name)
        if (os.path.exists(target)
                and os.path.getmtime(target) >= os.path.getmtime(source)):
            continue
        if not patch:
            shutil.copyfile(source, target + ".tmp")
        else:
            data = bytearray(open(source, "rb").read())
            if patch_import_name(data) != 1:
                sys.exit("%s: expected one import of %s" % (source, CRT_NAME.decode()))
            with open(target + ".tmp", "wb") as out:
                out.write(data)
        os.replace(target + ".tmp", target)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    stage(sys.argv[1], sys.argv[2])
