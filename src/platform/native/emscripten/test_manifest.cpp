// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_manifest — the web build's list of game files (manifest.h): which
 * lines the parser keeps, and the paths it refuses. */
#include "platform/native/emscripten/manifest.h"

#include <stdio.h>
#include <string.h>

static int s_failures = 0;

static void CHECK(bool cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static size_t parse(const char* text, std::vector<std::string>& files)
{
    return DataManifest_Parse(text, strlen(text), files);
}

int main()
{
    std::vector<std::string> files;

    size_t dropped = parse(
        "# lrr-portable web manifest\n"
        "LegoRR0.wad\n"
        "\n"
        "Data/Sounds/Streamed/a.wav\r\n"
        "LegoRR1.wad\n"
        "LegoRR0.wad\n"
        "Data/last.txt",
        files
    );
    CHECK(dropped == 0, "a clean manifest drops nothing");
    CHECK(files.size() == 4, "comments, blank lines and duplicates are skipped");
    CHECK(files.size() == 4 && files[0] == "LegoRR0.wad" && files[1] == "Data/Sounds/Streamed/a.wav" &&
              files[2] == "LegoRR1.wad" && files[3] == "Data/last.txt",
          "paths keep their order, case and spaces; CRLF and a missing final newline are fine");

    files.clear();
    dropped = parse(
        "../outside\n"
        "/rooted\n"
        "Data/../up\n"
        "Data//double\n"
        "Data/./dot\n"
        "dir/\n"
        "back\\slash\n"
        "ctl\x01\n"
        "Data/ok name.wav\n",
        files
    );
    CHECK(dropped == 8, "each unsafe path is counted");
    CHECK(files.size() == 1 && files[0] == "Data/ok name.wav", "only the safe path is kept");

    files.clear();
    CHECK(parse("", files) == 0 && files.empty(), "an empty manifest has no files");

    files.push_back("keep.wad");
    parse("keep.wad\nnew.wad\n", files);
    CHECK(files.size() == 2 && files[1] == "new.wad", "parsing appends, and skips what the list holds");

    CHECK(DataManifest_IsSafePath("Data/a.wav"), "a nested path is safe");
    CHECK(!DataManifest_IsSafePath(""), "the empty path is not");
    CHECK(!DataManifest_IsSafePath(".."), "'..' is not");

    if (s_failures == 0) {
        printf("PASS\n");
    }
    return s_failures == 0 ? 0 : 1;
}
