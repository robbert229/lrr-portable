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
