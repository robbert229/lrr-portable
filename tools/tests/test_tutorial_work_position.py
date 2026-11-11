# SPDX-License-Identifier: LGPL-3.0-or-later
"""Exercise the actual named-node/work-position owners with controlled pose leaves.

This checks object selection and failure propagation, not real COM transforms
or tutorial runtime acceptance. The fixture uses distinct roots and authored
nodes in two activities; replacing the named node with the root fails it.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(text, start):
    begin = text.index(start)
    brace = text.index("{", begin)
    depth = 1
    end = brace + 1
    while depth:
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
        end += 1
    return text[begin:end] + "\n"


class WorkPositionTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("c++"), "requires a native C++ compiler")
    def test_configured_current_activity_node_and_missing_resources(self):
        lwsc = (ROOT / "src/gods98/lwsc_scene.cpp").read_text()
        actor = (ROOT / "src/game/actor_scene.cpp").read_text()
        entity = (ROOT / "src/game/entity.cpp").read_text()
        bodies = (function(lwsc, "static int lwsc_native_name(") +
                  function(lwsc, "const char* lwsc_assembly_configured_name(") +
                  function(lwsc, "const LwscFrameRef* lwsc_instance_named_node(") +
                  function(actor, "void* actor_scene_motion_component(") +
                  function(actor, "void* actor_scene_named_node(") +
                  function(entity, "int entity_work_object_position_get("))
        template = (Path(__file__).parent / "fixtures/tutorial_work_position.inc").read_text()
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "owner.cpp"
            binary = Path(directory) / "owner"
            source.write_text(template.replace("/*EXTRACTED*/", bodies))
            compiled = subprocess.run(["c++", "-std=c++98", "-O2", "-Wall", "-Wextra",
                                       str(source), "-o", str(binary)], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("cases=15 failures=0", result.stdout)


if __name__ == "__main__":
    unittest.main()
