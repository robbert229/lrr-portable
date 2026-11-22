// SPDX-License-Identifier: LGPL-3.0-or-later
/* files.c — CRLF line reads, whole-file load callbacks and the mounted-path
 * rewrite. The existing stream and WAD fixture covers general I/O. */
#include <cstdlib>
#include <fstream>
#include <string>

#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

#include "gods98/files.h"

struct LoadNotice {
    std::string path;
    unsigned int size = 0;
    unsigned int loads = 0;
};

static void record_load(const char* path, unsigned int size, void* data)
{
    auto* notice = static_cast<LoadNotice*>(data);
    notice->path = path;
    notice->size = size;
    ++notice->loads;
}

int main()
{
    if (!CHECK(fixture_enter("files_dispatch_fixture") && fixture_mkdir("Data"), "create data fixture"))
        return lrr_test_result();
    const char text[] = "first\r\n\r\nlast";
    {
        std::ofstream file("Data/lines.txt", std::ios::binary);
        file.write(text, sizeof(text) - 1);
        if (!CHECK(file.good(), "write input lines")) return lrr_test_result();
    }
    file_initialise("files_dispatch_fixture", 0, "");

    File* file = file_open("lines.txt", "rb");
    if (CHECK(file != nullptr, "open input lines")) {
        char line[32] = {};
        CHECK_STREQ(file_get_line(line, sizeof(line), file), "first");
        CHECK_STREQ(file_get_line(line, sizeof(line), file), "");
        CHECK_STREQ(file_get_line(line, sizeof(line), file), "last");
        CHECK_EQ(file_get_line(line, sizeof(line), file), nullptr);
        CHECK_EQ(file_close(file), 0);
    }

    LoadNotice notice;
    file_set_load_callback(record_load, &notice);
    unsigned int size = 0;
    auto* data = static_cast<char*>(file_load_binary("lines.txt", &size));
    CHECK_EQ(size, static_cast<unsigned int>(sizeof(text) - 1));
    if (CHECK(data != nullptr, "load whole input")) {
        CHECK_EQ(std::memcmp(data, text, sizeof(text) - 1), 0);
        std::free(data);
    }
    CHECK_EQ(notice.loads, 1u);
    CHECK_EQ(notice.size, size);
    CHECK_STREQ(notice.path.c_str(), "lines.txt");
    CHECK_EQ(file_load_binary("absent.txt", nullptr), nullptr);
    CHECK_EQ(notice.loads, 1u);
    file_set_load_callback(nullptr, nullptr);

    CHECK_EQ(file_set_base_search_path("C:\\Game\\Data"), 1);
    char mounted[] = "c:\\game\\data\\Levels\\Map.txt";
    char unrelated[] = "D:\\Game\\Data\\Map.txt";
    CHECK_STREQ(file_get_wad_name(mounted), "Levels\\Map.txt");
    CHECK_STREQ(mounted, "c:\\game\\data\\Levels\\Map.txt");
    CHECK_EQ(file_get_wad_name(unrelated), unrelated + 0);
    CHECK_EQ(file_set_base_search_path(nullptr), 0);
    CHECK_EQ(file_get_wad_name(mounted), mounted + 0);
    return lrr_test_result();
}
