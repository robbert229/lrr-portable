// SPDX-License-Identifier: LGPL-3.0-or-later
/* wad.c — WWAD entry names, archive precedence, RNC expansion, and
 * concurrent entry handles on two small synthetic archives. */
#include <cstdint>
#include <fstream>
#include <string_view>

#include "gods98/wad.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

static void put32(std::ofstream& file, uint32_t value)
{
    unsigned char data[4];
    for (unsigned int i = 0; i < sizeof(data); ++i) {
        data[i] = static_cast<unsigned char>(value);
        value >>= 8;
    }
    file.write(reinterpret_cast<const char*>(data), sizeof(data));
}

static bool write_wad(const char* path, const char* name, std::string_view payload,
                      uint32_t unpacked, uint32_t flags)
{
    constexpr char dataName[] = "Data\\Different.bin";
    const std::size_t nameSize = std::strlen(name) + 1;
    std::ofstream file(path, std::ios::binary);
    file.write("WWAD", 4);
    put32(file, 1);
    file.write(name, nameSize);
    file.write(dataName, sizeof(dataName));
    put32(file, flags);
    put32(file, static_cast<uint32_t>(payload.size()));
    put32(file, unpacked);
    put32(file, static_cast<uint32_t>(8 + nameSize + sizeof(dataName) + 16));
    file.write(payload.data(), payload.size());
    file.close();
    return file.good();
}

static void check_entry(int handle, std::string_view expected)
{
    if (!CHECK(handle >= 0, "entry has a live handle")) return;
    CHECK_EQ(wad_file_length(handle), static_cast<int>(expected.size()));
    auto* data = static_cast<const char*>(wad_file_data(handle));
    if (CHECK(data != nullptr, "entry has data")) {
        CHECK_EQ(std::string_view(data, expected.size()), expected);
    }
}

int main()
{
    if (!CHECK(fixture_enter("wad_handles_fixture"), "create WAD fixture")) return lrr_test_result();
    const char rnc[] = {'R', 'N', 'C', 0, 0, 0, 0, 3, 0, 0, 0, 3,
                        0, 0, 0, 0, 0, 0, 'n', 'e', 'w'};
    if (!CHECK(write_wad("base.wad", "Shared.bin", "old content", 11, 0) &&
               write_wad("patch.wad", "shared.BIN", {rnc, sizeof(rnc)}, 3, 2),
               "write stored and packed archives")) return lrr_test_result();

    const int base = wad_load("base.wad");
    const int patch = wad_load("patch.wad");
    if (!CHECK(base >= 0 && patch > base, "both archives mounted in order")) return lrr_test_result();
    CHECK_EQ(wad_is_file_in_wad("SHARED.bin", base), 0);
    CHECK_EQ(wad_is_file_in_wad("shared.bin", -1), 0);
    CHECK_EQ(wad_is_file_in_wad("Data\\Different.bin", base), -1);
    CHECK_EQ(wad_file_open("absent.bin", -1), -1);

    const int original = wad_file_open("SHARED.bin", base);
    const int preferred = wad_file_open("SHARED.bin", -1);
    check_entry(original, "old content");
    check_entry(preferred, "new");
    CHECK(original != preferred, "concurrent opens have distinct handles");
    if (original >= 0) {
        wad_file_close(original);
        CHECK_EQ(wad_file_data(original), nullptr);
    }
    check_entry(preferred, "new");
    const int reopened = wad_file_open("shared.bin", patch);
    check_entry(reopened, "new");
    CHECK_EQ(reopened, original);
    if (reopened >= 0) wad_file_close(reopened);
    if (preferred >= 0 && preferred != reopened) wad_file_close(preferred);
    return lrr_test_result();
}
