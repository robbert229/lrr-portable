# SPDX-License-Identifier: LGPL-3.0-or-later
"""The release helpers (tools/release.py, tools/check_package.py) and the
release files they read (docs/releasing.md)."""
import importlib.util
import io
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
import zipfile

TOOLS = Path(__file__).resolve().parents[1]
ROOT = TOOLS.parent


def load(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


release = load("release")
check_package = load("check_package")

CHANGELOG = """# Changelog

## [Unreleased]

- Something coming.

## [0.2.0] - 2027-01-01

### Fixed
- A bug.

## [0.1.0] - 2026-12-01

First release.

[Unreleased]: https://example.com/compare/v0.2.0...HEAD
[0.2.0]: https://example.com/releases/tag/v0.2.0
"""


class VersionTests(unittest.TestCase):
    def test_reads_the_project_version(self):
        self.assertEqual(
            release.project_version("project(lrr-portable VERSION 1.2.3 LANGUAGES C CXX)\n"),
            "1.2.3")

    def test_rejects_a_version_that_is_not_semver(self):
        with self.assertRaises(release.ReleaseError):
            release.project_version("project(lrr-portable VERSION 1.2 LANGUAGES C)\n")

    def test_the_repository_version_is_semver(self):
        version = release.project_version((ROOT / "CMakeLists.txt").read_text())
        self.assertRegex(version, r"^\d+\.\d+\.\d+$")

    def test_the_changelog_has_notes_for_the_repository_version(self):
        version = release.project_version((ROOT / "CMakeLists.txt").read_text())
        notes = release.release_notes((ROOT / "CHANGELOG.md").read_text(), version)
        self.assertIn("own copy", notes)


class NotesTests(unittest.TestCase):
    def test_extracts_a_middle_section(self):
        self.assertEqual(release.release_notes(CHANGELOG, "0.2.0"), "### Fixed\n- A bug.\n")

    def test_stops_at_link_references(self):
        self.assertEqual(release.release_notes(CHANGELOG, "0.1.0"), "First release.\n")

    def test_missing_section(self):
        with self.assertRaises(release.ReleaseError):
            release.release_notes(CHANGELOG, "9.9.9")

    def test_empty_section(self):
        with self.assertRaises(release.ReleaseError):
            release.release_notes("## [1.0.0]\n\n## [0.9.0]\nx\n", "1.0.0")


class TagTests(unittest.TestCase):
    CMAKE = "project(lrr-portable VERSION 0.2.0 LANGUAGES C CXX)\n"

    def test_matching_tag(self):
        self.assertEqual(release.check_tag("v0.2.0", self.CMAKE, CHANGELOG), "0.2.0")

    def test_tag_without_v(self):
        with self.assertRaises(release.ReleaseError):
            release.check_tag("0.2.0", self.CMAKE, CHANGELOG)

    def test_tag_for_another_version(self):
        with self.assertRaises(release.ReleaseError):
            release.check_tag("v0.1.0", self.CMAKE, CHANGELOG)


def zip_package(names):
    data = io.BytesIO()
    with zipfile.ZipFile(data, "w") as archive:
        for name in names:
            archive.writestr(f"lrr-portable-0.1.0-Windows-x64/{name}", b"x")
    return data.getvalue()


class PackageTests(unittest.TestCase):
    def check(self, suffix, data, require=()):
        with tempfile.TemporaryDirectory() as work:
            path = Path(work) / f"package{suffix}"
            path.write_bytes(data)
            return check_package.problems(check_package.list_package(path), require)

    def test_a_windows_zip_passes(self):
        names = ["LegoRR.exe", "LICENSE.txt", "THIRD-PARTY-NOTICES.txt",
                 "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"]
        self.assertEqual(self.check(".zip", zip_package(names), ["vcruntime140.dll"]), [])

    def test_a_linux_tgz_passes(self):
        data = io.BytesIO()
        with tarfile.open(fileobj=data, mode="w:gz") as archive:
            for name in ["bin/LegoRR", "share/doc/lrr-portable/LICENSE",
                         "share/doc/lrr-portable/THIRD-PARTY-NOTICES.txt",
                         "share/applications/io.github.robbert229.LegoRR.desktop",
                         "share/metainfo/io.github.robbert229.LegoRR.metainfo.xml",
                         "share/icons/hicolor/scalable/apps/io.github.robbert229.LegoRR.svg"]:
                info = tarfile.TarInfo(f"lrr-portable-0.1.0-Linux-x86_64/{name}")
                info.size = 1
                archive.addfile(info, io.BytesIO(b"x"))
        self.assertEqual(self.check(".tar.gz", data.getvalue()), [])

    def test_missing_notices_fail(self):
        found = self.check(".zip", zip_package(["LegoRR.exe", "LICENSE.txt"]))
        self.assertEqual(found, ["missing the third-party notices"])

    def test_missing_required_file_fails(self):
        names = ["LegoRR.exe", "LICENSE.txt", "THIRD-PARTY-NOTICES.txt"]
        self.assertEqual(self.check(".zip", zip_package(names), ["vcruntime140.dll"]),
                         ["missing vcruntime140.dll"])

    def test_game_data_fails(self):
        names = ["LegoRR.exe", "LICENSE.txt", "THIRD-PARTY-NOTICES.txt",
                 "LegoRR0.wad", "Data/Lego.cfg", "Music/Track02.ogg"]
        found = self.check(".zip", zip_package(names))
        self.assertEqual(len(found), 3, found)
        self.assertTrue(all("game data" in problem for problem in found), found)

    def test_branding_fails(self):
        names = ["LegoRR.exe", "LICENSE.txt", "THIRD-PARTY-NOTICES.txt", "isle.png"]
        self.assertEqual(self.check(".zip", zip_package(names)),
                         ["isle.png: isle-portable branding"])

    def test_unknown_file_fails(self):
        names = ["LegoRR.exe", "LICENSE.txt", "THIRD-PARTY-NOTICES.txt", "splash.png"]
        found = self.check(".zip", zip_package(names))
        self.assertEqual(len(found), 1)
        self.assertIn("not on the allow-list", found[0])

    def test_a_mounted_disk_image_passes(self):
        with tempfile.TemporaryDirectory() as work:
            root = Path(work)
            for name in ["LegoRR.app/Contents/Info.plist",
                         "LegoRR.app/Contents/MacOS/LegoRR",
                         "LegoRR.app/Contents/Resources/AppIcon.icns",
                         "LegoRR.app/Contents/Resources/LICENSE",
                         "LegoRR.app/Contents/Resources/THIRD-PARTY-NOTICES.txt",
                         "LegoRR.app/Contents/_CodeSignature/CodeResources",
                         "LICENSE", "THIRD-PARTY-NOTICES.txt"]:
                (root / name).parent.mkdir(parents=True, exist_ok=True)
                (root / name).write_text("x")
            # The image's link to /Applications is listed, not followed.
            (root / "Applications").symlink_to(TOOLS, target_is_directory=True)
            files = check_package.list_directory(root)
            self.assertIn("Applications", files)
            self.assertEqual(check_package.problems(files), [])

    def test_command_line_exit_status(self):
        with tempfile.TemporaryDirectory() as work:
            path = Path(work) / "p.zip"
            path.write_bytes(zip_package(["LegoRR.exe"]))
            result = subprocess.run([sys.executable, str(TOOLS / "check_package.py"), str(path)],
                                    capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("missing the licence", result.stdout)


class ReleaseFilesTests(unittest.TestCase):
    def test_notices_name_every_fetched_dependency(self):
        notices = (ROOT / "THIRD-PARTY-NOTICES.txt").read_text()
        cmake = (ROOT / "CMakeLists.txt").read_text()
        for name in ("SDL", "iniparser", "miniaudio", "stb_vorbis", "miniwin"):
            self.assertIn(name, notices)
        # The SDL commit the notices name is the one CMake fetches.
        sdl = cmake.split('set(LRR_SDL3_GIT_TAG "', 1)[1].split('"', 1)[0]
        self.assertIn(sdl, notices)
        ini = cmake.split('GIT_REPOSITORY "https://gitlab.com/iniparser/iniparser.git"', 1)[1]
        self.assertIn(ini.split('GIT_TAG "', 1)[1].split('"', 1)[0], notices)
        mini = cmake.split("miniaudio/archive/refs/tags/", 1)[1].split(".tar.gz", 1)[0]
        self.assertIn(f"({mini})", notices)

    def test_release_workflow_only_drafts(self):
        workflow = (ROOT / ".github/workflows/release.yml").read_text()
        self.assertIn("--draft", workflow)
        self.assertNotIn("--draft=false", workflow)
        self.assertNotIn("make_latest", workflow)
        # The one job that can write is the one that drafts the release.
        self.assertEqual(workflow.count("contents: write"), 1)


if __name__ == "__main__":
    unittest.main()
