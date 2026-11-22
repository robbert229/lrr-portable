#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Checks a release package's contents (docs/releasing.md).

    tools/check_package.py [--require NAME]... PACKAGE...

A PACKAGE is a .zip, an .apk, a .tar.gz, an .AppImage (extracted with its own
--appimage-extract, so only on a host that can run it), a .dmg (listed with
7z), a .flatpak bundle (imported with flatpak and listed with ostree), or a
directory such as a mounted disk image.

Each package must
  - hold the licence (LICENSE or LICENSE.txt) and THIRD-PARTY-NOTICES.txt;
  - hold nothing but the files below (ALLOWED). The list is closed so that
    game data, LEGO art or another project's branding cannot slip into a
    package unnoticed: a new kind of file fails the check until it is added
    here, in review.
--require NAME also demands a file of that name (case-insensitive, with
fnmatch wildcards), for example the Visual C++ runtime in an MSVC ZIP.

Exits 0 when every package passes, 1 otherwise, and prints each problem.
"""
import argparse
import fnmatch
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile

APP_ID = r"io\.github\.robbert229\.LegoRR"

# Files a package may hold, as regular expressions over its relative paths
# (with the single top-level directory of a ZIP, TGZ or disk image removed),
# matched case-insensitively.
ALLOWED = [
    # The program.
    r"(usr/|files/)?(bin/)?LegoRR(\.exe)?",
    # The licence and the notices, anywhere.
    r"(.*/)?LICENSE(\.txt)?",
    r"(.*/)?THIRD-PARTY-NOTICES\.txt",
    # Windows: a shared SDL3 and the Visual C++ runtime.
    r"SDL3\.dll",
    r"(msvcp|vcruntime|concrt|vccorlib)140(_\w+)?\.dll",
    # Linux: the desktop entry, AppStream metadata and icon.
    rf"(usr/|files/|export/)?share/applications/{APP_ID}\.desktop",
    rf"(usr/|files/|export/)?share/metainfo/{APP_ID}\.metainfo\.xml",
    rf"(usr/|files/|export/)?share/icons/hicolor/[^/]+/apps/{APP_ID}\.(svg|png)",
    # The AppImage's root and the libraries linuxdeploy bundles.
    r"AppRun",
    r"\.DirIcon",
    rf"{APP_ID}\.(svg|desktop)",
    r"usr/lib/[^/]+\.so(\.[0-9]+)*",
    # The Flatpak: its metadata, flatpak-builder's manifest and AppStream
    # compose output.
    r"metadata",
    r"files/manifest\.json",
    rf"files/share/app-info/xmls/{APP_ID}\.xml\.gz",
    rf"files/share/app-info/icons/flatpak/[^/]+/{APP_ID}\.png",
    # Android (an APK): the manifest, code, resources, the native libraries
    # per ABI, the signature, and the notices under assets/.
    r"AndroidManifest\.xml",
    r"classes[0-9]*\.dex",
    r"resources\.arsc",
    r"res/(drawable|mipmap)[^/]*/[^/]+\.(png|webp|xml)",
    r"lib/(arm64-v8a|armeabi-v7a|x86|x86_64)/lib(LegoRR|SDL3|c\+\+_shared)\.so",
    r"META-INF/(MANIFEST\.MF|[^/]+\.(SF|RSA|EC|DSA)|com/android/build/gradle/app-metadata\.properties)",
    r"assets/(LICENSE|THIRD-PARTY-NOTICES\.txt)",
    # Nintendo 3DS: the homebrew .3dsx and the installable .cia.
    r"LegoRR\.(3dsx|cia)",
    # macOS: the bundle, the disk image's Applications link and HFS+ entries.
    r"LegoRR\.app/Contents/(Info\.plist|PkgInfo|MacOS/LegoRR)",
    r"LegoRR\.app/Contents/Resources/AppIcon\.icns",
    r"LegoRR\.app/Contents/_CodeSignature/CodeResources",
    r"LegoRR\.app/Contents/Frameworks/[^/]+\.dylib",
    r"Applications",
    r"\.HFS\+ Private Directory Data.*",
    r"\[HFS\+ Private Data\].*",
    r"\.DS_Store",
    r"\.fseventsd(/.*)?",
    r"\.Trashes(/.*)?",
]
ALLOWED_RE = re.compile("|".join(f"(?:{p})" for p in ALLOWED), re.I)

# Reported with a reason before the allow-list, for the files most likely
# to arrive by mistake.
FORBIDDEN = [
    (re.compile(r"(^|/)[^/]*\.wad$", re.I), "game data (a WAD archive)"),
    (re.compile(r"(^|/)data/", re.I), "game data (a Data directory)"),
    (re.compile(r"(^|/)music/track[0-9]+\.", re.I), "game data (a music track)"),
    (re.compile(r"\.(avi|flh|lws|lwo|bmp|wav|cfg|nrm|sav|osf)$", re.I),
     "game data (a game file type)"),
    (re.compile(r"isle|legoisland", re.I), "isle-portable branding"),
]

REQUIRED = [
    ("the licence", re.compile(r"(^|/)LICENSE(\.txt)?$", re.I)),
    ("the third-party notices", re.compile(r"(^|/)THIRD-PARTY-NOTICES\.txt$", re.I)),
]


def strip_top(paths):
    """Removes the one top-level directory every path shares, if any."""
    paths = [p for p in paths if p]
    tops = {PurePosixPath(p).parts[0] for p in paths}
    if len(tops) == 1 and all(len(PurePosixPath(p).parts) > 1 for p in paths):
        return [str(PurePosixPath(*PurePosixPath(p).parts[1:])) for p in paths]
    return paths


def list_directory(root):
    root = Path(root)
    files = []
    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        here = Path(dirpath)
        # A symbolic link to a directory (the disk image's Applications) is a
        # file here: it is listed, not followed.
        for name in list(dirnames):
            if (here / name).is_symlink():
                dirnames.remove(name)
                filenames.append(name)
        for name in filenames:
            files.append((here / name).relative_to(root).as_posix())
    return files


def list_zip(path):
    with zipfile.ZipFile(path) as archive:
        return strip_top([n for n in archive.namelist() if not n.endswith("/")])


def list_tar(path):
    with tarfile.open(path) as archive:
        return strip_top([m.name.lstrip("./") for m in archive.getmembers() if not m.isdir()])


def list_appimage(path):
    with tempfile.TemporaryDirectory() as work:
        subprocess.run([os.path.abspath(path), "--appimage-extract"], cwd=work,
                       check=True, stdout=subprocess.DEVNULL,
                       env=dict(os.environ, APPIMAGE_EXTRACT_AND_RUN="1"))
        return list_directory(Path(work) / "squashfs-root")


def list_dmg(path):
    seven = shutil.which("7z") or shutil.which("7zz")
    if not seven:
        raise RuntimeError("7z is needed to list a disk image")
    out = subprocess.run([seven, "l", "-slt", "-ba", str(path)], check=True,
                         capture_output=True, text=True).stdout
    files = []
    entry = {}
    for line in out.splitlines() + [""]:
        if not line.strip():
            if entry.get("Path") and entry.get("Folder") != "+":
                files.append(entry["Path"])
            entry = {}
            continue
        key, _, value = line.partition(" = ")
        entry[key] = value
    return strip_top(files)


def list_flatpak(path):
    for tool in ("flatpak", "ostree"):
        if not shutil.which(tool):
            raise RuntimeError(f"{tool} is needed to list a Flatpak bundle")
    with tempfile.TemporaryDirectory() as work:
        repo = os.path.join(work, "repo")
        subprocess.run(["ostree", "init", f"--repo={repo}", "--mode=archive"], check=True)
        subprocess.run(["flatpak", "build-import-bundle", repo, str(path)], check=True,
                       stdout=subprocess.DEVNULL)
        refs = subprocess.run(["ostree", f"--repo={repo}", "refs"], check=True,
                              capture_output=True, text=True).stdout.split()
        if len(refs) != 1:
            raise RuntimeError(f"expected one ref in the bundle, found {refs}")
        out = subprocess.run(["ostree", f"--repo={repo}", "ls", "-R", refs[0]], check=True,
                             capture_output=True, text=True).stdout
    files = []
    for line in out.splitlines():
        fields = line.split(None, 4)
        if len(fields) == 5 and not fields[0].startswith("d"):
            name = fields[4].split(" -> ")[0]
            files.append(name.lstrip("/"))
    return files


def list_package(path):
    path = Path(path)
    name = path.name.lower()
    if path.is_dir():
        return list_directory(path)
    if name.endswith((".zip", ".apk")):
        return list_zip(path)
    if name.endswith((".tar.gz", ".tgz")):
        return list_tar(path)
    if name.endswith(".appimage"):
        return list_appimage(path)
    if name.endswith(".dmg"):
        return list_dmg(path)
    if name.endswith(".flatpak"):
        return list_flatpak(path)
    raise RuntimeError(f"{path}: not a package this tool can read")


def problems(files, require=()):
    """Returns the problems with a package that holds these files."""
    found = []
    for path in files:
        for pattern, reason in FORBIDDEN:
            if pattern.search(path):
                found.append(f"{path}: {reason}")
                break
        else:
            if not ALLOWED_RE.fullmatch(path):
                found.append(f"{path}: not on the allow-list in tools/check_package.py")
    for what, pattern in REQUIRED:
        if not any(pattern.search(p) for p in files):
            found.append(f"missing {what}")
    for name in require:
        if not any(fnmatch.fnmatch(PurePosixPath(p).name.lower(), name.lower()) for p in files):
            found.append(f"missing {name}")
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--require", action="append", default=[], metavar="NAME",
                        help="a file name (fnmatch) the package must also hold")
    parser.add_argument("packages", nargs="+", metavar="PACKAGE")
    args = parser.parse_args(argv)
    failed = False
    for package in args.packages:
        try:
            found = problems(list_package(package), args.require)
        except (RuntimeError, OSError, subprocess.CalledProcessError,
                zipfile.BadZipFile, tarfile.TarError) as error:
            found = [str(error)]
        if found:
            failed = True
            print(f"FAIL {package}")
            for problem in found:
                print(f"  {problem}")
        else:
            print(f"ok   {package}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
