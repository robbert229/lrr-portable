# SPDX-License-Identifier: LGPL-3.0-or-later
"""The packaging recipes (packaging/, issue #922) agree with one another.

The packages themselves are built by CI; these checks catch a recipe that
drifts from the application id, the executable name or the icon.
"""
from pathlib import Path
import re
import struct
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
PACKAGING = ROOT / "packaging"


def cmake_variable(name):
    text = (PACKAGING / "CMakeLists.txt").read_text()
    match = re.search(r'^set\(%s "(.*)"\)$' % name, text, re.M)
    assert match, name
    return match.group(1)


def configure(text):
    """Substitutes the @VAR@ placeholders the way configure_file does."""
    values = {
        "LRR_APP_ID": cmake_variable("LRR_APP_ID"),
        "LRR_APP_NAME": cmake_variable("LRR_APP_NAME"),
        "LRR_APP_SUMMARY": cmake_variable("LRR_APP_SUMMARY"),
        "LRR_APP_SPDX": cmake_variable("LRR_APP_SPDX"),
        "LRR_APP_URL": cmake_variable("LRR_APP_URL"),
        "LRR_SEMANTIC_VERSION": "0.1.0",
        "LRR_BUILD_DATE": "2026-11-21",
    }
    return re.sub(r"@(\w+)@", lambda m: values[m.group(1)], text)


class AppIdentityTests(unittest.TestCase):
    def setUp(self):
        self.app_id = cmake_variable("LRR_APP_ID")

    def test_app_id_is_reverse_dns(self):
        self.assertRegex(self.app_id, r"^[a-z]+(\.[A-Za-z0-9_]+){2,}$")

    def test_flatpak_uses_the_app_id_and_the_executable(self):
        manifest = (PACKAGING / "linux/flatpak" / f"{self.app_id}.yml").read_text()
        self.assertRegex(manifest, rf"(?m)^id: {re.escape(self.app_id)}$")
        # The game opens <executable name>0.wad, so only LegoRR finds its data.
        self.assertRegex(manifest, r"(?m)^command: LegoRR$")

    def test_desktop_entry(self):
        text = configure((PACKAGING / "linux/lrr-portable.desktop.in").read_text())
        entries = dict(
            line.split("=", 1) for line in text.splitlines() if "=" in line
        )
        self.assertEqual(entries["Icon"], self.app_id)
        self.assertEqual(entries["StartupWMClass"], self.app_id)
        self.assertEqual(entries["Exec"], "LegoRR")
        self.assertEqual(entries["TryExec"], "LegoRR")
        self.assertTrue(entries["Categories"].endswith(";"))

    def test_metainfo(self):
        text = configure((PACKAGING / "linux/lrr-portable.metainfo.xml.in").read_text())
        component = ET.fromstring(text)
        self.assertEqual(component.findtext("id"), self.app_id)
        self.assertEqual(
            component.find("launchable").text, f"{self.app_id}.desktop"
        )
        self.assertEqual(component.findtext("provides/binary"), "LegoRR")
        self.assertEqual(component.findtext("project_license"), "LGPL-3.0-or-later")
        # No package holds game data.
        self.assertIn("contains no game data", " ".join(component.find("description").itertext()))

    def test_macos_plist(self):
        text = (PACKAGING / "macos/Info.plist.in").read_text()
        self.assertIn("@MACOSX_BUNDLE_GUI_IDENTIFIER@", text)
        cmake = (PACKAGING / "macos/CMakeLists.txt").read_text()
        self.assertIn('set(MACOSX_BUNDLE_GUI_IDENTIFIER "${LRR_APP_ID}")', cmake)
        verify = (PACKAGING / "macos/verify.sh").read_text()
        self.assertIn(f'"{self.app_id}"', verify)

    def test_appimage_run_script_starts_legorr(self):
        run = (PACKAGING / "linux/appimage/AppRun").read_text()
        self.assertIn('exec "${HERE}/usr/bin/LegoRR" "$@"', run)

    def test_appimage_tools_are_pinned(self):
        build = (PACKAGING / "linux/appimage/build.sh").read_text()
        for name in ("LINUXDEPLOY", "APPIMAGETOOL"):
            self.assertRegex(build, rf"(?m)^{name}_SHA256=[0-9a-f]{{64}}$")
            self.assertNotIn("/continuous/", build)


class IconTests(unittest.TestCase):
    def test_svg_is_well_formed(self):
        root = ET.parse(PACKAGING / "icons/lrr-portable.svg").getroot()
        self.assertTrue(root.tag.endswith("svg"))
        self.assertEqual(root.get("viewBox"), "0 0 512 512")

    def test_icns_holds_the_expected_png_sizes(self):
        data = (PACKAGING / "macos/AppIcon.icns").read_bytes()
        self.assertEqual(data[:4], b"icns")
        self.assertEqual(struct.unpack(">I", data[4:8])[0], len(data))
        sizes = {}
        offset = 8
        while offset < len(data):
            kind = data[offset:offset + 4].decode("ascii")
            length = struct.unpack(">I", data[offset + 4:offset + 8])[0]
            png = data[offset + 8:offset + length]
            self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n", kind)
            sizes[kind] = struct.unpack(">II", png[16:24])
            offset += length
        self.assertEqual(offset, len(data))
        self.assertEqual(
            sizes,
            {"ic07": (128, 128), "ic08": (256, 256), "ic09": (512, 512), "ic10": (1024, 1024)},
        )


class LicenceTests(unittest.TestCase):
    def test_recipes_carry_spdx_headers(self):
        for path in PACKAGING.rglob("*"):
            if path.suffix in {".icns"} or not path.is_file():
                continue
            self.assertIn("SPDX-License-Identifier", path.read_text()[:400], path)


if __name__ == "__main__":
    unittest.main()
