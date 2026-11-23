#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Release helpers for the release workflow (docs/releasing.md).

    tools/release.py version            print the version from CMakeLists.txt
    tools/release.py check-tag TAG      fail unless TAG is v<version> and the
                                        changelog has notes for that version
    tools/release.py notes VERSION      print CHANGELOG.md's notes for VERSION
    tools/release.py prerelease         print true for a pre-release version,
                                        false otherwise

project(VERSION) and LRR_VERSION_PRERELEASE in CMakeLists.txt are the one
place the version is set: MAJOR.MINOR.PATCH, plus "-<label>" for a
pre-release such as 1.0.0-beta1. A release is tagged v<version>, and CHANGELOG.md has a "## [<version>]"
section, whose body is the release's notes.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SEMVER = re.compile(r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")
# A Semantic Versioning pre-release: dot-separated identifiers of
# [0-9A-Za-z-], numeric ones without leading zeros.
PRERELEASE_IDENTIFIER = re.compile(r"^(0|[1-9][0-9]*|[0-9]*[A-Za-z-][0-9A-Za-z-]*)$")


class ReleaseError(Exception):
    pass


def project_version(cmake_text):
    match = re.search(r"^project\(\s*lrr-portable\s+VERSION\s+([^\s)]+)", cmake_text, re.M)
    if not match:
        raise ReleaseError("CMakeLists.txt has no project(lrr-portable VERSION ...)")
    version = match.group(1)
    if not SEMVER.match(version):
        raise ReleaseError(f"project VERSION {version} is not MAJOR.MINOR.PATCH")
    label = re.search(r'^set\(\s*LRR_VERSION_PRERELEASE\s+"([^"]*)"\s*\)', cmake_text, re.M)
    if label and label.group(1):
        if not all(PRERELEASE_IDENTIFIER.match(part) for part in label.group(1).split(".")):
            raise ReleaseError(
                f"LRR_VERSION_PRERELEASE {label.group(1)} is not a Semantic Versioning pre-release")
        version += "-" + label.group(1)
    return version


def is_prerelease(version):
    return "-" in version


def release_notes(changelog_text, version):
    """Returns the body of the changelog's section for this version."""
    lines = changelog_text.splitlines()
    heading = re.compile(r"^## \[" + re.escape(version) + r"\](\s|$)")
    for start, line in enumerate(lines):
        if heading.match(line):
            break
    else:
        raise ReleaseError(f"CHANGELOG.md has no '## [{version}]' section")
    body = []
    for line in lines[start + 1:]:
        # The next release's section, or the link references at the end.
        if line.startswith("## ") or re.match(r"^\[[^\]]+\]: ", line):
            break
        body.append(line)
    text = "\n".join(body).strip()
    if not text:
        raise ReleaseError(f"CHANGELOG.md's section for {version} is empty")
    return text + "\n"


def check_tag(tag, cmake_text, changelog_text):
    version = project_version(cmake_text)
    if tag != f"v{version}":
        raise ReleaseError(
            f"tag {tag} does not match project(VERSION) {version}; "
            f"tag v{version}, or change the version first")
    release_notes(changelog_text, version)
    return version


def main(argv):
    cmake = (ROOT / "CMakeLists.txt").read_text()
    changelog = (ROOT / "CHANGELOG.md").read_text()
    try:
        if argv[:1] == ["version"] and len(argv) == 1:
            print(project_version(cmake))
        elif argv[:1] == ["check-tag"] and len(argv) == 2:
            print(check_tag(argv[1], cmake, changelog))
        elif argv[:1] == ["prerelease"] and len(argv) == 1:
            print("true" if is_prerelease(project_version(cmake)) else "false")
        elif argv[:1] == ["notes"] and len(argv) == 2:
            sys.stdout.write(release_notes(changelog, argv[1]))
        else:
            sys.stderr.write(__doc__)
            return 2
    except ReleaseError as error:
        print(f"release.py: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
