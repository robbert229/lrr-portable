#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""List the game files the web build streams (issue #908).

The web build registers the files of `LegoRR/LegoRR.manifest` on a WasmFS
fetch backend (src/platform/native/emscripten/manifest.h). This writes that
manifest from a game directory, at deploy time, so the build holds no list of
files and no game data:

    tools/web_manifest.py /path/to/game -o deploy/LegoRR/LegoRR.manifest

then publish the game directory's files beside it, at <host>/LegoRR/. The
manifest lists the WADs in the game directory's top level, and everything
under Data, except what the web build cannot use or keeps itself:

  * Data/Saves: the player's saves live in the browser (OPFS), not on the host.
  * movies (*.avi): the web build has no FFmpeg, so the game skips them, and
    they are most of the game's size.

`--exclude GLOB` (repeatable, matched with fnmatch against the path relative
to the game directory, and against each file's name) leaves out more, and
`--include-movies` keeps the movies. The output is sorted, so the same
directory gives the same manifest. Paths keep the case the directory has,
which is the case the host must serve.
"""
import argparse
import fnmatch
import os
import sys

HEADER = "# lrr-portable web manifest: the files streamed from <host>/LegoRR\n"
DEFAULT_EXCLUDES = ["Data/Saves/*"]
MOVIE_EXCLUDES = ["*.avi"]


def is_excluded(path, patterns):
    name = path.rsplit("/", 1)[-1]
    return any(
        fnmatch.fnmatchcase(path.lower(), pattern.lower())
        or fnmatch.fnmatchcase(name.lower(), pattern.lower())
        for pattern in patterns
    )


def data_files(game_dir, excludes):
    """The manifest's paths for `game_dir`, sorted, '/'-separated."""
    files = []
    for entry in os.scandir(game_dir):
        if entry.is_file() and entry.name.lower().endswith(".wad"):
            files.append(entry.name)
    # The data directory's own name is whatever case the directory has.
    for entry in os.scandir(game_dir):
        if entry.is_dir() and entry.name.lower() == "data":
            for root, _dirs, names in os.walk(entry.path):
                for name in names:
                    full = os.path.join(root, name)
                    files.append(os.path.relpath(full, game_dir).replace(os.sep, "/"))
    return sorted(path for path in files if not is_excluded(path, excludes))


def manifest_text(game_dir, excludes):
    return HEADER + "".join(path + "\n" for path in data_files(game_dir, excludes))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("game_dir", help="a game directory: LegoRR0.wad, LegoRR1.wad and Data")
    parser.add_argument("-o", "--output", help="the manifest file to write (default: standard output)")
    parser.add_argument("--exclude", action="append", default=[], metavar="GLOB",
                        help="leave out files matching GLOB (repeatable)")
    parser.add_argument("--include-movies", action="store_true",
                        help="keep the movies (*.avi), which the web build cannot play")
    args = parser.parse_args(argv)

    if not os.path.isdir(args.game_dir):
        parser.error(f"{args.game_dir} is not a directory")
    excludes = DEFAULT_EXCLUDES + ([] if args.include_movies else MOVIE_EXCLUDES) + args.exclude
    text = manifest_text(args.game_dir, excludes)
    if not any(line.lower().endswith(".wad") for line in text.splitlines()):
        parser.error(f"{args.game_dir} holds no WAD archives")

    if args.output:
        os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
        with open(args.output, "w", newline="\n") as out:
            out.write(text)
        print(f"{len(text.splitlines()) - 1} files listed in {args.output}", file=sys.stderr)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
