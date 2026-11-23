// SPDX-License-Identifier: LGPL-3.0-or-later
/* rnc.c — expand stored and method-2 payloads, exercise a backreference,
 * reject unsupported headers, and check method-1 canonical code order. */
#include <cstdlib>
#include <string_view>

#include "gods98/rnc.h"
#include "test/lrr_test.h"

static void check_expand(const unsigned char* packed, std::string_view expected)
{
    const auto size = static_cast<long>(expected.size());
    CHECK_EQ(wad_expand_payload(packed, nullptr), size);
    void* unpacked = nullptr;
    CHECK_EQ(wad_expand_payload(packed, &unpacked), size);
    if (CHECK(unpacked != nullptr, "decoder returns an allocated buffer")) {
        CHECK_EQ(std::string_view(static_cast<char*>(unpacked), expected.size()), expected);
        std::free(unpacked);
    }
}

static void check_codes()
{
    RNC_HuffEntry table[5] = {};
    table[0].bitDepth = 3;
    table[1].bitDepth = 1;
    table[2].bitDepth = 3;
    table[4].bitDepth = 2;
    wad_huffman_canonical_codes_build(table, 5);
    CHECK_EQ(table[1].code, 0u); // Canonical 0.
    CHECK_EQ(table[4].code, 1u); // Canonical 10, reversed to 01.
    CHECK_EQ(table[0].code, 3u); // Canonical 110, reversed to 011.
    CHECK_EQ(table[2].code, 7u); // Canonical 111.
    CHECK_EQ(table[3].bitDepth, 0u);
    CHECK_EQ(wad_reverse_low_bits(0xbe, 3), 3u);
    wad_huffman_table_clear(table, 5);
    for (const auto& entry : table) {
        CHECK_EQ(entry.bitDepth, 0u);
        CHECK_EQ(entry.code, 0u);
    }
}

int main()
{
    alignas(4) const unsigned char stored[] = {
        'R', 'N', 'C', 0, 0, 0, 0, 4, 0, 0, 0, 4,
        0, 0, 0, 0, 0, 0, 'a', 0, 'b', 0xff,
    };
    check_expand(stored, std::string_view("a\0b\xff", 4));

    alignas(4) const unsigned char method2[] = {
        'R', 'N', 'C', 2, 0, 0, 0, 4, 0, 0, 0, 6,
        0, 0, 0, 0, 0, 1,
        // Prefix 00, two literals, then a length-two copy at distance two.
        // The rest of the control bits and zero byte end the block.
        0x0d, 'A', 'B', 1, 0xe0, 0,
    };
    check_expand(method2, "ABAB");
    char guarded[] = "<____>";
    CHECK_EQ(wad_payload_decode_dispatch(method2, guarded + 1), 0);
    CHECK_STREQ(guarded, "<ABAB>");

    alignas(4) unsigned char bad[sizeof(stored)];
    std::memcpy(bad, stored, sizeof(bad));
    bad[3] = 3;
    CHECK_EQ(wad_payload_decode_dispatch(bad, guarded + 1), -2);
    void* unpacked = nullptr;
    CHECK_EQ(wad_expand_payload(bad, &unpacked), 0l);
    CHECK_EQ(unpacked, nullptr);
    bad[0] = 'X';
    CHECK_EQ(wad_payload_decode_dispatch(bad, guarded + 1), -1);
    CHECK_STREQ(guarded, "<ABAB>");

    bad[0] = 'R';
    bad[3] = 0;
    bad[6] = 1;
    bad[7] = 2;
    CHECK_EQ(wad_expand_payload(bad, nullptr), 258l);
    check_codes();
    return lrr_test_result();
}
