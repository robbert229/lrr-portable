#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Run Reccmp with complete JSON listings, including exact matches."""
import runpy
import sys
import reccmp.compare.report as report
from reccmp.compare.diff import raw_diff_to_udiff

original = report.get_udiff_for_entity

def full_diff(entity):
    if entity.rdiff is not None:
        return raw_diff_to_udiff(entity.rdiff, grouped=False)
    return original(entity)

report.get_udiff_for_entity = full_diff
script = sys.argv.pop(1)
sys.argv[0] = script
runpy.run_path(script, run_name='__main__')
