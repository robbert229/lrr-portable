// SPDX-License-Identifier: LGPL-3.0-or-later
/* Ordinary LF/CRLF and EOF handling on the real standard-file dispatch.
 * No resource discovery or application startup is needed. */
#include <fstream>
#include <initializer_list>

#include "platform/native/test_fixture.h"
#include "gods98/files.h"
#include "test/lrr_test.h"

static void check_lines(const char* path, const char* text,
                        std::initializer_list<const char*> expected)
{
    {
        std::ofstream out(path, std::ios::binary);
        out << text;
        out.close();
        if (!CHECK(out.good(), "write synthetic text file")) return;
    }
    File file = {};
    file.type = FileSystem_Std;
    file.stream.std = SDL_IOFromFile(path, "rb");
    if (!CHECK(file.stream.std != nullptr, "open synthetic text file")) return;

    char line[32] = {};
    for (const char* value : expected) {
        CHECK_EQ(file_get_line(line, sizeof(line), &file), line + 0);
        CHECK_STREQ(line, value);
    }
    // A successful empty line is distinct from EOF. Repeated reads after
    // EOF must continue returning NULL.
    for (int i = 0; i < 2; ++i) {
        CHECK_EQ(file_get_line(line, sizeof(line), &file), nullptr);
        CHECK_EQ(line[0], '\0');
    }
    CHECK(SDL_CloseIO(file.stream.std), "close synthetic text file");
}

int main()
{
    if (!CHECK(fixture_enter("file_line_reader_fixture"), "isolated directory"))
        return lrr_test_result();
    check_lines("mixed.txt", "first\r\n\r\nsecond\n\nlast",
                {"first", "", "second", "", "last"});
    check_lines("empty.txt", "", {});
    check_lines("newline.txt", "\n", {""});
    check_lines("terminated.txt", "last\r\n", {"last"});
    return lrr_test_result();
}
