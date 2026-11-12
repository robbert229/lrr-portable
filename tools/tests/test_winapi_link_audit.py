# SPDX-License-Identifier: LGPL-3.0-or-later
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import link_order
import winapi_link_audit as audit

LIST = """\
lrr_link(unit   GAME-001 game          game/advisor.c)
lrr_link(legacy 004777a0 program       platform/winapi/entry.cpp winapi)
lrr_link(tail   -        game          game/game.cpp)
lrr_link(tail   -        gods98        gods98/container_native.cpp native)
lrr_link(tail   -        program       main.cpp native)
lrr_link(tail   -        program       platform/native/entry.cpp native)
set(LRR_LINK_LIBRARIES libc.lib oldnames.lib kernel32.lib)
"""

RSP = ("CMakeFiles/program.dir/src/version.rc.res "
       "CMakeFiles/link_001_advisor.dir/src/game/advisor.c.obj "
       "CMakeFiles/link_002_entry.dir/src/platform/winapi/entry.cpp.obj "
       "CMakeFiles/link_003_game.dir/src/game/game.cpp.obj\n")
LINK = ("link /nologo @CMakeFiles/program.dir/objects1.rsp /out:LegoRR.exe "
        "/implib:LegoRR.lib libc.lib oldnames.lib kernel32.lib /DEBUG\n")
MAP = """\
 LegoRR

 Start         Length     Name                   Class
 0001:00000000 00000100H .text                   CODE
 0002:00000000 00000040H .rdata                  DATA

  Address         Publics by Value              Rva+Base   Lib:Object

 0001:00000000       _advisor_init              00401000 f advisor.c.obj
 0001:00000040       _advisor_port_hook         00401040 f advisor.c.obj
 0001:00000060       _WinMain@16                00401060 f entry.cpp.obj
 0001:00000080       ?game_context_init@@YAXPAUGameContext@@@Z 00401080 f game.cpp.obj
 0001:000000c0       _malloc                    004010c0 f libc:malloc.obj
 0001:000000f0       _Sleep@4                   004010f0 f kernel32:KERNEL32.dll
 0002:00000010       _table                     00402010   advisor.c.obj
"""


def entries(text=LIST):
    return link_order.parse(text)


class CheckListTests(unittest.TestCase):
    def test_clean_list_passes(self):
        self.assertEqual(audit.check_list(entries(), link_order.libraries(LIST)), [])

    def test_untagged_native_path_fails(self):
        text = LIST.replace("platform/native/entry.cpp native)", "platform/native/entry.cpp)")
        problems = audit.check_list(entries(text), [])
        self.assertEqual(len(problems), 1)
        self.assertIn("platform/native/entry.cpp", problems[0])

    def test_untagged_native_only_source_fails(self):
        text = LIST.replace("main.cpp native)", "main.cpp)")
        problems = audit.check_list(entries(text), [])
        self.assertEqual(len(problems), 1)
        self.assertIn("LegorrrMain", problems[0])

    def test_miniwin_pattern_fails(self):
        text = LIST + "lrr_link(tail   -        gods98        gods98/lwsc_miniwin.cpp)\n"
        self.assertTrue(audit.check_list(entries(text), []))

    def test_sdl_library_fails(self):
        problems = audit.check_list(entries(), ["libc.lib", "SDL3.lib"])
        self.assertEqual(problems, [f"{link_order.LIST}: LRR_LINK_LIBRARIES names SDL3.lib"])

    def test_repository_list_passes(self):
        text = (audit.ROOT / link_order.LIST).read_text(encoding="utf-8")
        self.assertEqual(audit.check_list(link_order.parse(text), link_order.libraries(text)), [])


class CheckLinkTests(unittest.TestCase):
    def check(self, rsp=RSP, link=LINK, map_text=MAP):
        objects, libraries = audit.link_inputs(rsp, link)
        return audit.check_link(entries(), objects, libraries, map_text)

    def test_clean_link_passes(self):
        self.assertEqual(self.check(), [])

    def test_link_inputs(self):
        objects, libraries = audit.link_inputs(RSP, LINK)
        self.assertEqual(len(objects), 4)
        self.assertEqual(libraries, ["libc.lib", "oldnames.lib", "kernel32.lib"])

    def test_native_platform_object_fails(self):
        rsp = RSP + " CMakeFiles/link_009_frameloop.dir/src/platform/native/frameloop.cpp.obj"
        problems = self.check(rsp=rsp)
        self.assertTrue(any("platform/native/frameloop.cpp" in p for p in problems))

    def test_native_tagged_object_fails(self):
        rsp = RSP + " CMakeFiles/link_009_container_native.dir/src/gods98/container_native.cpp.obj"
        self.assertTrue(any("container_native" in p for p in self.check(rsp=rsp)))

    def test_native_only_source_object_fails(self):
        rsp = RSP + " CMakeFiles/link_009_main.dir/src/main.cpp.obj"
        self.assertTrue(any("main.cpp.obj comes from" in p for p in self.check(rsp=rsp)))

    def test_sdl_library_fails(self):
        problems = self.check(link=LINK.replace("kernel32.lib", "kernel32.lib SDL3.lib"))
        self.assertEqual(problems, ["library SDL3.lib is a native-backend library"])

    def test_sdl_symbol_fails(self):
        map_text = MAP.replace("_advisor_port_hook        ", "_SDL_GetTicks             ")
        problems = self.check(map_text=map_text)
        self.assertEqual(len(problems), 1)
        self.assertIn("_SDL_GetTicks", problems[0])

    def test_miniwin_object_fails(self):
        map_text = MAP.replace("f libc:malloc.obj", "f miniwin:ddraw.obj")
        self.assertTrue(any("miniwin:ddraw.obj" in p for p in self.check(map_text=map_text)))

    def test_native_entry_symbol_fails(self):
        map_text = MAP.replace("?game_context_init@@YAXPAUGameContext@@@Z",
                               "?LegorrrRun@@YAHXZ")
        problems = self.check(map_text=map_text)
        self.assertEqual(len(problems), 1)
        self.assertIn("native entry chain", problems[0])

    def test_unlisted_object_fails(self):
        map_text = MAP.replace("f game.cpp.obj", "f creature_scene.cpp.obj")
        problems = self.check(map_text=map_text)
        self.assertEqual(problems, ["map object creature_scene.cpp.obj is not a winapi "
                                    "entry of the link-order list"])


class CategoryTests(unittest.TestCase):
    def test_categories_with_annotations(self):
        cats = audit.text_categories(MAP, entries(), {0x401000})
        self.assertEqual(cats["original_annotated"], {"bytes": 0x40, "functions": 1})
        self.assertEqual(cats["port_unit"], {"bytes": 0x20, "functions": 1})
        self.assertEqual(cats["port_legacy"], {"bytes": 0x20, "functions": 1})
        self.assertEqual(cats["port_tail"], {"bytes": 0x40, "functions": 1})
        self.assertEqual(cats["libc"], {"bytes": 0x30, "functions": 1})
        self.assertEqual(cats["import_thunks"], {"bytes": 0x10, "functions": 1})
        self.assertEqual(cats["total"]["bytes"], 0x100)

    def test_categories_without_report(self):
        cats = audit.text_categories(MAP, entries(), None)
        self.assertNotIn("original_annotated", cats)
        self.assertEqual(cats["port_unit"]["bytes"], 0x60)

    def test_symbol_base(self):
        self.assertEqual(audit.symbol_base("?LegorrrRun@@YAHXZ"), "LegorrrRun")
        self.assertEqual(audit.symbol_base("_WinMain@16"), "WinMain")
        self.assertEqual(audit.symbol_base("_game_layer_name"), "game_layer_name")


class CommandTests(unittest.TestCase):
    def build_dir(self, tmp: Path, rsp=RSP) -> Path:
        program = tmp / "CMakeFiles/program.dir"
        program.mkdir(parents=True)
        (program / "objects1.rsp").write_text(rsp, encoding="utf-8")
        (program / "link.txt").write_text(LINK, encoding="utf-8")
        (tmp / "LegoRR.map").write_text(MAP, encoding="latin-1")
        report = tmp / "reccmp-report"
        report.mkdir()
        (report / "results.json").write_text(json.dumps(
            {"data": [{"address": "0x401000", "recomp": "0x401000"}]}), encoding="utf-8")
        return tmp

    def test_check_link_command(self):
        repo_entries = entries()
        original = audit.link_order.load
        audit.link_order.load = lambda root=None: repo_entries
        try:
            with tempfile.TemporaryDirectory() as tmp:
                build = self.build_dir(Path(tmp))
                out = Path(tmp) / "out.json"
                self.assertEqual(audit.main(["check-link", "--build", str(build),
                                             "--json", str(out)]), 0)
                data = json.loads(out.read_text(encoding="utf-8"))
                self.assertEqual(data["problems"], [])
                self.assertEqual(data["text"]["original_annotated"]["bytes"], 0x40)
            with tempfile.TemporaryDirectory() as tmp:
                build = self.build_dir(Path(tmp), RSP + " CMakeFiles/x.dir/src/main.cpp.obj")
                self.assertEqual(audit.main(["check-link", "--build", str(build)]), 1)
        finally:
            audit.link_order.load = original

    def test_missing_build(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(audit.main(["check-link", "--build", tmp]), 2)


if __name__ == "__main__":
    unittest.main()
