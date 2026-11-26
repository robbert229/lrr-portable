# SPDX-License-Identifier: LGPL-3.0-or-later
"""The packaging recipes (packaging/, issue #922) agree with one another.

The packages themselves are built by CI; these checks catch a recipe that
drifts from the application id, the executable name or the icon.
"""
import json
from pathlib import Path
import re
import struct
import sys
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


def config_table():
    """The game's settings, by RecompLauncher's key (section/key, the key
    in lowercase, as iniparser writes it): the rows of kTable in
    src/platform/native/config.cpp, each its kind, its default as the file
    holds it, and a choice's names."""
    native = ROOT / "src/platform/native"
    header = (native / "config.h").read_text()
    source = (native / "config.cpp").read_text()
    defines = dict(re.findall(r'#define (LRR_CONFIG_\w+) "([^"]*)"', header))
    enums = {name: int(value) for name, value in re.findall(r"\b(LRR_\w+) = (\d+),", header)}
    names = {
        name: re.findall(r'"([^"]*)"', values)
        for name, values in re.findall(r"constexpr const char\* (k\w+Names)\[\] = \{([^}]*)\};", source)
    }
    body = re.search(r"constexpr Row kTable\[\] = \{(.*?)\n\};", source, re.S).group(1)
    table = {}
    for kind, args in re.findall(r"\b(Switch|Choice|Number|Text|Directory)\(LRR_SETTING_\w+, ([^)]*)\)", body):
        key, *rest = [arg.strip() for arg in args.split(",")]
        key = defines[key] if key in defines else key.strip('"')
        section, name = key.split(":", 1)
        row = {"kind": kind, "names": None}
        if kind == "Switch":
            row["default"] = rest[0]
        elif kind == "Choice":
            row["names"] = names[rest[0]]
            # The default is an enum (config.h) or a plain index.
            index = enums[rest[1]] if rest[1] in enums else int(rest[1])
            row["default"] = row["names"][index]
        elif kind == "Number":
            row["default"] = rest[0]
        else:
            # A directory's NULL default is the save directory, which a
            # launcher leaves to the game.
            row["default"] = rest[0].strip('"') if rest[0].startswith('"') else None
        table[section + "/" + name.lower()] = row
    return table


# The spellings a launcher writes for a switch: the canonical words the
# game reads (config.h, LRR_Setting), not iniparser's first-letter fallback.
SWITCH_WORDS = {"true", "on", "yes", "1", "false", "off", "no", "0", "original"}


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


class RecompLauncherTests(unittest.TestCase):
    """packaging/recomplauncher.json (RecompLauncher's manifest) agrees with
    what the game reads and how the packages are laid out."""

    def setUp(self):
        self.manifest = json.loads((PACKAGING / "recomplauncher.json").read_text())
        self.table = config_table()

    def test_names_this_repository(self):
        url = cmake_variable("LRR_APP_URL")
        self.assertEqual(url, "https://github.com/" + self.manifest["repo"])
        self.assertEqual(self.manifest["format"], 1)

    def test_asks_for_the_game_folder_and_passes_the_options_the_entry_reads(self):
        (game,) = self.manifest["inputs"]
        self.assertEqual(game["id"], "game")
        self.assertEqual(game["formats"], [{"type": "folder"}])
        self.assertEqual(game["check"], {"file": "LegoRR0.wad"})
        args = self.manifest["start"]["args"]
        self.assertEqual(args, ["--game", "{input:game}", "--data", "{data}"])
        entry = (ROOT / "src/platform/native/entry.cpp").read_text()
        for option in ("--game", "--data"):
            self.assertIn(f'strcmp(argv[i], "{option}") == 0', entry)

    def test_starts_the_one_program_the_game_opens_its_wads_for(self):
        # The game opens <program name>0.wad, so every entry starts LegoRR
        # (the AppImage through its AppRun).
        for program in self.manifest["start"]["program"]:
            self.assertRegex(program, r"^(LegoRR\.exe|LegoRR\.app|bin/LegoRR|[\w.-]+\.AppImage)$")

    def test_reads_the_settings_table(self):
        # One row per LRR_Setting, so a row the parser misses fails here.
        header = (ROOT / "src/platform/native/config.h").read_text()
        enum = re.search(r"enum LRR_Setting \{(.*?)\};", header, re.S).group(1)
        count = len(re.findall(r"^\s+LRR_SETTING_\w+,", enum, re.M))
        self.assertEqual(len(self.table), count)
        self.assertEqual(self.table["tweaks/training save"]["default"], "prompt")
        self.assertEqual(self.table["legorr/3d device"]["default"], "")
        self.assertEqual(self.table["cheats/unlimited_oxygen"], {"kind": "Switch", "names": None, "default": "false"})

    def written(self):
        """Each (key, value) the manifest writes, and where."""
        for setting in self.manifest["settings"]:
            self.assertEqual(setting.get("in", "main"), "main")
            for choice in setting["choices"]:
                for key, value in choice["set"].items():
                    yield setting["id"], key, value
            for key, value in setting["defaults"].items():
                yield setting["id"] + " defaults", key, value
        for key, value in self.manifest["config"][0]["new"].items():
            yield "new", key, value

    def test_settings_write_keys_and_values_the_game_reads(self):
        written = list(self.written())
        self.assertTrue(written)
        for where, key, value in written:
            self.assertIn(key, self.table, where)
            row = self.table[key]
            if row["kind"] == "Switch":
                self.assertIn(value.lower(), SWITCH_WORDS, f"{where}: {key}")
            elif row["kind"] == "Choice":
                self.assertIn(value.lower(), row["names"], f"{where}: {key}")
            elif row["kind"] == "Number":
                self.assertRegex(value, r"^-?\d+$", f"{where}: {key}")

    def test_defaults_are_the_games(self):
        # What the launcher shows for a missing key, and writes to a new
        # file, is what the game reads for it.
        for where, key, value in self.written():
            if where == "new" or where.endswith(" defaults"):
                self.assertEqual(value, self.table[key]["default"], f"{where}: {key}")

    def test_every_choice_of_a_setting_sets_the_same_keys(self):
        for setting in self.manifest["settings"]:
            keysets = {frozenset(choice["set"]) for choice in setting["choices"]}
            self.assertEqual(len(keysets), 1, setting["id"])
            self.assertEqual(keysets.pop(), frozenset(setting["defaults"]), setting["id"])

    def test_keeps_everything_it_writes_in_the_data_folder(self):
        (config,) = self.manifest["config"]
        self.assertEqual(config["file"], [{"path": "{data}/LegoRR.ini"}])
        self.assertEqual(self.manifest["saves"], ["{data}/Saves"])
        for place in self.manifest["folders"]["saves"]:
            self.assertTrue(place["path"].startswith("{data}/"))

    def test_the_packages_carry_it_at_the_top(self):
        cmake = (PACKAGING / "CMakeLists.txt").read_text()
        self.assertIn('install(FILES recomplauncher.json DESTINATION ".")', cmake)
        self.assertIn('if(WIN32 OR APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")', cmake)
        # On macOS the launcher installs a ZIP with the .app at its top, not
        # the disk image.
        self.assertIn("set(CPACK_GENERATOR DragNDrop ZIP)", cmake)
        hosts = (ROOT / ".github/workflows/native-hosts.yml").read_text()
        self.assertIn("build/dist/lrr-portable-*.zip", hosts)
        release = (ROOT / ".github/workflows/release.yml").read_text()
        for arch in ("arm64", "x86_64"):
            self.assertIn(f'"lrr-portable-$VERSION-Darwin-{arch}.zip"', release)
        # An AppImage installs under /usr; the manifest belongs at its root.
        build = (PACKAGING / "linux/appimage/build.sh").read_text()
        self.assertIn('mv "$APPDIR/usr/recomplauncher.json" "$APPDIR/recomplauncher.json"', build)
        sys.path.insert(0, str(ROOT / "tools"))
        try:
            import check_package
        finally:
            sys.path.pop(0)
        self.assertEqual(check_package.problems(["recomplauncher.json", "LICENSE", "THIRD-PARTY-NOTICES.txt"]), [])


class BundledFFmpegTests(unittest.TestCase):
    """The FFmpeg the macOS packages bundle (packaging/macos/ffmpeg) stays
    LGPL, and its notices and source go with it."""

    def setUp(self):
        self.build = (PACKAGING / "macos/ffmpeg/build.sh").read_text()
        self.version = re.search(r'^FFMPEG_VERSION="([^"]+)"', self.build, re.M).group(1)
        self.sha256 = re.search(r'^FFMPEG_SHA256="([0-9a-f]{64})"', self.build, re.M).group(1)

    def test_no_licence_changing_configure_flag(self):
        flags = self.build.split("CONFIGURE_FLAGS=(", 1)[1].split(")", 1)[0]
        for flag in ("--enable-gpl", "--enable-version3", "--enable-nonfree"):
            self.assertNotIn(flag, flags)
        for flag in ("--disable-autodetect", "--disable-everything", "--enable-shared"):
            self.assertIn(flag, flags)

    def test_notices_name_the_bundled_source(self):
        notices = (ROOT / "THIRD-PARTY-NOTICES.txt").read_text()
        self.assertIn(f"https://ffmpeg.org/releases/ffmpeg-{self.version}.tar.xz", notices)
        self.assertIn(self.sha256, notices)
        self.assertIn("LGPLv2.1", notices)
        self.assertIn("FFmpeg-COPYING.LGPLv2.1.txt", notices)

    def test_the_macos_packages_bundle_it_and_the_release_carries_its_source(self):
        hosts = (ROOT / ".github/workflows/native-hosts.yml").read_text()
        self.assertIn('packaging/macos/ffmpeg/build.sh build "$RUNNER_TEMP/ffmpeg"', hosts)
        self.assertIn('-DLRR_FFMPEG_DIR="$RUNNER_TEMP/ffmpeg"', hosts)
        release = (ROOT / ".github/workflows/release.yml").read_text()
        self.assertIn("packaging/macos/ffmpeg/build.sh fetch release", release)
        cmake = (PACKAGING / "CMakeLists.txt").read_text()
        self.assertIn("FFmpeg-COPYING.LGPLv2.1.txt", cmake)

    def test_a_package_with_ffmpeg_needs_its_licence(self):
        sys.path.insert(0, str(ROOT / "tools"))
        try:
            import check_package
        finally:
            sys.path.pop(0)
        base = ["LICENSE", "THIRD-PARTY-NOTICES.txt", "LegoRR.app/Contents/MacOS/LegoRR"]
        lib = ["LegoRR.app/Contents/Frameworks/libavcodec.63.dylib"]
        licence = ["LegoRR.app/Contents/Resources/FFmpeg-COPYING.LGPLv2.1.txt"]
        self.assertEqual(check_package.problems(base + lib + licence), [])
        self.assertTrue(check_package.problems(base + lib))
        self.assertEqual(check_package.problems(base), [])


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
        action = (ROOT / ".github/actions/setup-sdl3/action.yml").read_text()
        self.assertRegex(action, rf"SDL_VERSION={re.escape(version)}\b")
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
        # carry the header (the SVGs, regenerate.py). JSON has no comments,
        # and RecompLauncher's schema takes no extra keys: recomplauncher.json
        # is under the repository's licence.
        for path in PACKAGING.rglob("*"):
            if path.suffix in {".icns", ".png", ".wav", ".jar", ".json"} or not path.is_file():
                continue
            # Build output and Gradle's caches in a local Android build.
            if {"build", ".gradle", ".cxx", "libs"} & set(path.relative_to(PACKAGING).parts):
                continue
            self.assertIn("SPDX-License-Identifier", path.read_text()[:1000], path)


if __name__ == "__main__":
    unittest.main()
