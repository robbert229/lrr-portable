#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Run Reccmp with complete JSON listings, including exact matches."""
import json
import os
import runpy
import sys
import reccmp.compare.report as report
from reccmp.compare.diff import raw_diff_to_udiff
from reccmp.compare import Compare
from reccmp.types import ImageId

import reccmp_map_statics
import reccmp_vtordisp_scope

# Deviation (scoring only): the vtordisp byte-pattern scan runs on the original
# image only, as in match_function.py; on the rebuilt image it matches jump
# tables (finding 616). RECCMP_VTORDISP_SCOPE=0 turns it off.
if os.environ.get("RECCMP_VTORDISP_SCOPE", "1") != "0":
    reccmp_vtordisp_scope.install()

# LIBC's static functions get their rebuilt-side names from the link map, so
# markers and data-source rows that name them map (#447, finding 643).
# RECCMP_MAP_STATICS=0 turns it off.
reccmp_map_statics.install()

_original_report = Compare.to_report
def export_entities(self, *args, **kwargs):
    result = _original_report(self, *args, **kwargs)
    path = os.environ.get("RECCMP_ENTITIES")
    if path:
        entities = [{"orig": e.orig_addr, "recomp": e.recomp_addr,
                     "type": e.entity_type, "size": e.size(ImageId.RECOMP)}
                    for e in self._db.get_all()]
        with open(path, "w", encoding="utf-8") as output:
            json.dump(entities, output)
    return result
Compare.to_report = export_entities

original = report.get_udiff_for_entity

def full_diff(entity):
    if entity.rdiff is not None:
        return raw_diff_to_udiff(entity.rdiff, grouped=False)
    return original(entity)

report.get_udiff_for_entity = full_diff
script = sys.argv.pop(1)
sys.argv[0] = script
runpy.run_path(script, run_name='__main__')
