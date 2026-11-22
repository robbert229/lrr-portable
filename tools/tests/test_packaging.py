# SPDX-License-Identifier: LGPL-3.0-or-later
"""The packaging recipes (packaging/, issue #922) agree with one another.

The packages themselves are built by CI; these checks catch a recipe that
drifts from the application id, the executable name or the icon.
"""
from pathlib import Path
import re
import struct
import unittest
import wave
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
        "LRR_RELEASE_TYPE": "stable",
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

    def test_svg_root_is_within_the_mime_sniff_window(self):
        # shared-mime-info recognises SVG only by "<svg" in the first 256
        # bytes; past that, appstreamcli compose in the Flatpak build reads
        # the icon as plain XML and fails with icon-file-read-error.
        data = (PACKAGING / "icons/lrr-portable.svg").read_bytes()
        self.assertIn(b"<svg", data[:256])

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


class Nintendo3DSTests(unittest.TestCase):
    """packaging/3ds's assets are in the forms smdhtool and bannertool take."""

    def png_size(self, name):
        data = (PACKAGING / "3ds" / name).read_bytes()
        self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n", name)
        return struct.unpack(">II", data[16:24])

    def test_icon_and_banner_sizes(self):
        self.assertEqual(self.png_size("icon.png"), (48, 48))
        self.assertEqual(self.png_size("banner.png"), (256, 128))

    def test_banner_sound_is_short_pcm(self):
        with wave.open(str(PACKAGING / "3ds/banner.wav")) as sound:
            self.assertEqual(sound.getsampwidth(), 2)
            self.assertLessEqual(sound.getnframes() / sound.getframerate(), 3.0)

    def test_cia_asks_for_the_new_3ds_memory_mode(self):
        text = (PACKAGING / "3ds/template.rsf").read_text()
        self.assertRegex(text, r"(?m)^\s*SystemModeExt\s*:\s*124MB\b")
        self.assertRegex(text, r'(?m)^\s*Title\s*:\s*"LRR Portable"')


class AndroidTests(unittest.TestCase):
    """The Gradle project (packaging/android, issue #927)."""

    ANDROID = PACKAGING / "android"

    def setUp(self):
        self.gradle = (self.ANDROID / "app/build.gradle").read_text()

    def test_package_is_the_app_id(self):
        app_id = cmake_variable("LRR_APP_ID")
        self.assertIn(f"applicationId '{app_id}'", self.gradle)
        self.assertIn(f"namespace '{app_id}'", self.gradle)
        manifest = (self.ANDROID / "app/src/main/AndroidManifest.xml").read_text()
        self.assertIn('android:name=".LegoRRActivity"', manifest)
        activity = self.ANDROID / "app/src/main/java" / app_id.replace(".", "/") / "LegoRRActivity.java"
        self.assertIn(f"package {app_id};", activity.read_text())

    def test_activity_loads_the_program(self):
        app_id = cmake_variable("LRR_APP_ID")
        activity = self.ANDROID / "app/src/main/java" / app_id.replace(".", "/") / "LegoRRActivity.java"
        # CMakeLists.txt's program is libLegoRR.so on Android; SDL runs the
        # last library's SDL_main.
        self.assertRegex(activity.read_text(), r'"SDL3", "LegoRR" \}')

    def test_app_name(self):
        strings = ET.parse(self.ANDROID / "app/src/main/res/values/strings.xml").getroot()
        self.assertEqual(strings.findtext("string[@name='app_name']"), cmake_variable("LRR_APP_NAME"))

    def test_default_abis(self):
        match = re.search(r"findProperty\('abiFilters'\) \?: '([^']*)'", self.gradle)
        self.assertIsNotNone(match)
        self.assertEqual(match.group(1).split(","), ["arm64-v8a", "x86_64"])

    def test_downloads_are_pinned(self):
        self.assertRegex(self.gradle, r"(?m)^def sdlDevelSha256 = '[0-9a-f]{64}'$")
        # The AAR is SDL's release, the same one CMakeLists.txt fetches.
        version = re.search(r"(?m)^def sdlVersion = '([0-9.]+)'$", self.gradle).group(1)
        workflow = (ROOT / ".github/workflows/native-build.yml").read_text()
        self.assertRegex(workflow, rf"SDL_VERSION: ['\"]?{re.escape(version)}['\"]?")
        wrapper = (self.ANDROID / "gradle/wrapper/gradle-wrapper.properties").read_text()
        self.assertRegex(wrapper, r"(?m)^distributionSha256Sum=[0-9a-f]{64}$")

    def test_no_signing_secrets(self):
        self.assertNotIn("signingConfig", self.gradle)

    def test_launcher_icons(self):
        res = self.ANDROID / "app/src/main/res"
        for density, scale in [("mdpi", 1), ("hdpi", 1.5), ("xhdpi", 2), ("xxhdpi", 3), ("xxxhdpi", 4)]:
            for name, dp in [("ic_launcher", 48), ("ic_launcher_foreground", 108)]:
                data = (res / f"mipmap-{density}" / f"{name}.png").read_bytes()
                self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n", name)
                size = round(dp * scale)
                self.assertEqual(struct.unpack(">II", data[16:24]), (size, size), f"{density}/{name}")


class LicenceTests(unittest.TestCase):
    def test_recipes_carry_spdx_headers(self):
        # The rendered images and sound are generated from sources that
        # carry the header (the SVGs, regenerate.py).
        for path in PACKAGING.rglob("*"):
            if path.suffix in {".icns", ".png", ".wav", ".jar"} or not path.is_file():
                continue
            # Build output and Gradle's caches in a local Android build.
            if {"build", ".gradle", ".cxx", "libs"} & set(path.relative_to(PACKAGING).parts):
                continue
            self.assertIn("SPDX-License-Identifier", path.read_text()[:1000], path)


if __name__ == "__main__":
    unittest.main()
