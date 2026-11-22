// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web build's list of game files (issue #908): the counterpart of
 * isle-portable's g_files, which the web build registers on the WasmFS
 * fetch backend (filesystem.cpp). isle-portable compiles its list in; this
 * one is generated from a game directory at deploy time by
 * tools/web_manifest.py, and served beside the data as
 * LegoRR/LegoRR.manifest, so one build serves any copy of the data.
 *
 * The file is text, one path per line, relative to the data directory,
 * with '/' separators and the case the host serves. Blank lines and lines
 * that start with '#' are skipped. Plain C++, so a native test covers it. */
#ifndef LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_MANIFEST_H
#define LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_MANIFEST_H

#include <cstddef>
#include <string>
#include <vector>

/* The manifest's name in the data directory on the host. */
#define LRR_DATA_MANIFEST_NAME "LegoRR.manifest"

/* Parses `p_length` bytes of manifest text into `p_files`, in order,
 * skipping duplicates. A line that is not a safe relative path ("..", a
 * leading '/', a backslash, a control character) is dropped, so a host
 * cannot make the page register a file outside the data directory.
 * Returns the number of such lines dropped. */
size_t DataManifest_Parse(const char* p_text, size_t p_length, std::vector<std::string>& p_files);

/* Whether `p_path` is a safe relative path (the rule above). */
bool DataManifest_IsSafePath(const std::string& p_path);

#endif /* LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_MANIFEST_H */
