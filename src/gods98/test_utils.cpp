// SPDX-License-Identifier: LGPL-3.0-or-later
/* utils.c — in-place token boundaries, formatted escapes, substring
 * positions, hash normalization and the three-valued boolean parser. */
#include <cstdlib>

#include "gods98/utils.h"
#include "test/lrr_test.h"

int main()
{
    char* fields[8] = {};
    char empty[] = "";
    CHECK_EQ(sound_variant_split(empty, fields, "::"), 0);
    CHECK_EQ(whitespace_tokenize(empty, fields), 0);

    char variants[] = "one:part::two::";
    CHECK_EQ(sound_variant_split(variants, fields, "::"), 3);
    CHECK_STREQ(fields[0], "one:part");
    CHECK_STREQ(fields[1], "two");
    CHECK_STREQ(fields[2], "");

    char words[] = " \tone\r\n  two\t";
    CHECK_EQ(whitespace_tokenize(words, fields), 4);
    CHECK_STREQ(fields[0], "");
    CHECK_STREQ(fields[1], "one");
    CHECK_STREQ(fields[2], "two");
    CHECK_STREQ(fields[3], "");

    char source[] = "copy";
    char* copy = string_duplicate(source);
    source[0] = 'C';
    CHECK_STREQ(copy, "copy");
    std::free(copy);
    char* message = formatted_text_escape_decode_and_emit("Score_%d\\nNext\\t", 42);
    CHECK_STREQ(message, "Score 42\nNext\\t");
    std::free(message);

    const char haystack[] = "aBaBA";
    CHECK_EQ(case_insensitive_substring_find(haystack, "bAb"), haystack + 1);
    CHECK_EQ(case_insensitive_substring_find(haystack, "aBaBAx"), nullptr);
    CHECK_EQ(case_insensitive_substring_find(haystack, ""), nullptr);

    const unsigned char compact[] = "AB";
    const unsigned char spaced[] = " \ta b";
    CHECK_EQ(game_string_hash_compute(compact, 0, 0),
             game_string_hash_compute(spaced, 1, 1));
    CHECK(game_string_hash_compute(spaced, 0, 1) != game_string_hash_compute(compact, 0, 0),
          "whitespace contributes when requested");
    CHECK(game_string_hash_compute(spaced, 1, 0) != game_string_hash_compute(compact, 0, 0),
          "case contributes when requested");

    for (const char* yes : {"yEs", "TRUE", "on"}) CHECK_EQ(block_get_bool(yes), 1);
    for (const char* no : {"No", "false", "OFF"}) CHECK_EQ(block_get_bool(no), 0);
    for (const char* unknown : {"", "1", "yes please"}) CHECK_EQ(block_get_bool(unknown), 2);
    return lrr_test_result();
}
