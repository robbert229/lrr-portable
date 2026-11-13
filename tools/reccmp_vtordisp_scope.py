#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Scan only the original image for vtordisp thunks (finding 616).

Deviation (scoring only): ADR 0005 entropy rule 4.

Reccmp finds vtordisp thunks by a byte pattern in every code region of both
images: ``2B 49 xx E9 <rel32>`` (``sub ecx, [ecx+xx]; jmp``) and its two
longer forms. In the rebuilt image that scan has nothing real to find, since
the rebuild's functions come from its PDB, but it can match data that sits in
``.text``. In-function jump tables hold absolute addresses, so two adjacent
entries such as ``0x00492be4`` and ``0x004925e9`` spell ``2B 49 00 E9 25 49
00 FB``. Reccmp then adds an 8-byte VTORDISP entity inside the function, and
its "is this an address" test for the jump-table operands that follow hits
that entity, not the function. The tables lose their names and a
byte-identical function scores below 100% (0x00489dd0, 100.00 -> 99.62%,
when G98-025 moved the rebuilt dispatcher to 0x00492590).

The original image (LegoRR.exe) has no vtordisp thunk; the scan of it is
kept, so a real one would still be found there. ``tools/match_function.py``
already scans only the original image, so this also gives the full run the
single-function loop's scope. RECCMP_VTORDISP_SCOPE=0 turns it off, to
measure what it changes. Storage and Reccmp's package are untouched.
"""
from __future__ import annotations

# Set on the wrapper, so repeated installs are no-ops.
_INSTALLED = "_lrr_vtordisp_scope_installed"


def original_only(create, original_id):
    """Wrap Reccmp's ``create_analysis_vtordisps(db, img_id, binfile)`` so
    that it runs for ``original_id`` only."""
    if getattr(create, _INSTALLED, False):
        return create

    def create_for_original(db, img_id, binfile):
        if img_id != original_id:
            return None
        return create(db, img_id, binfile)

    setattr(create_for_original, _INSTALLED, True)
    create_for_original.__wrapped__ = create
    return create_for_original


def install() -> None:
    """Patch the name Reccmp's Compare calls (reccmp.compare.core)."""
    import reccmp.compare.core as core
    from reccmp.types import ImageId

    core.create_analysis_vtordisps = original_only(
        core.create_analysis_vtordisps, ImageId.ORIG)
