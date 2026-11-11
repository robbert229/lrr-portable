// SPDX-License-Identifier: LGPL-3.0-only
// Corpus text primitives (findings 397/12): duplicate, substring find,
// bounded length, and the `::` config-path builder.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/heap.hpp"
#include "gods98/string.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

static void test_duplicate(void)
{
    const char* src = "MiniFigureTypes";
    char* dup = string_duplicate(src);
    CHECK(dup != NULL, "duplicate allocated");
    CHECK(dup != src && strcmp(dup, src) == 0, "duplicate is an equal copy");
    engine_heap_free(dup);

    char* empty = string_duplicate("");
    CHECK(empty != NULL && empty[0] == '\0', "empty string duplicates to a byte");
    engine_heap_free(empty);
}

static void test_find_substring(void)
{
    const char* hay = "LegoRR Main Menu::MainMenuFull MenuCount";
    CHECK(string_find_substring(hay, "::") == strstr(hay, "::"),
          "two-byte needle finds the separator");
    CHECK(string_find_substring(hay, "Menu") == strstr(hay, "Menu"),
          "multi-byte needle finds the first hit");
    CHECK(string_find_substring(hay, "M") == strstr(hay, "M"),
          "single-byte needle finds the first byte");
    CHECK(string_find_substring(hay, "") == hay, "empty needle yields the start");
    CHECK(string_find_substring(hay, "Nope") == NULL, "missing needle answers NULL");
    CHECK(string_find_substring("abc", "c") == strstr("abc", "c"),
          "one-byte hit at the tail");
    CHECK(string_find_substring("ab", "abc") == NULL, "needle longer than hay");

    /* A 128-byte filler exercises the scan past any single word. */
    char big[260];
    memset(big, 'x', sizeof big);
    big[259] = '\0';
    big[200] = 'Z';
    CHECK(string_find_substring(big, "Z") == big + 200, "scan walks to a far hit");
    big[200] = 'x';
    CHECK(string_find_substring(big, "Z") == NULL, "no hit in the far filler");
}

static void test_length_bounded(void)
{
    const char* s = "Lego*";
    CHECK(string_length_bounded(s, 16) == 5, "NUL within the bound counts bytes");
    CHECK(string_length_bounded(s, 3) == 3, "no NUL in the bound answers the bound");
    CHECK(string_length_bounded(s, 5) == 5, "exact bound finds the NUL at the end");
    CHECK(string_length_bounded(s, 0) == 0, "zero bound reads nothing");
    CHECK(string_length_bounded("", 16) == 0, "empty string length zero");
}

int main(void)
{
    test_duplicate();
    test_find_substring();
    test_length_bounded();
    /* CaseInsensitiveSubstringFind (2026-10-25). */
    {
        const char* hay = "Hello World!";
        const char* hit = case_insensitive_substring_find(hay, "lo wo");
        CHECK(hit == hay + 3, "the fold find lands the start");
        CHECK(case_insensitive_substring_find(hay, "nope") == NULL,
              "the miss answers null");
        CHECK(case_insensitive_substring_find("", "x") == NULL
                  && case_insensitive_substring_find("abc", "") == NULL,
              "the corpus gate answers null");
        CHECK(case_insensitive_substring_find("ab", "abc") == NULL,
              "the tail overrun does not match");
    }

    /* StringCaseSubstringFind (2026-10-25): the __strnicmp-shape walk with
     * the index/length outputs, the gate, and the guards. */
    {
        char hay[] = "Hello World";
        uint32_t idx = 123u;
        size_t len = 123u;
        char* hit = string_case_substring_find(hay, "WORLD", &idx, &len);
        CHECK(hit == hay + 6, "the fold walk lands the match");
        CHECK(idx == 6u && len == 5u, "the outputs report the spot");

        /* the first match wins, zero-overlap scan */
        char* first = string_case_substring_find(hay, "L", &idx, &len);
        CHECK(first == hay + 2 && idx == 2u && len == 1u,
              "the first folded hit answers first");

        /* the outputs ride only on a match */
        idx = 77u;
        len = 77u;
        CHECK(string_case_substring_find(hay, "nope", &idx, &len) == NULL,
              "the miss answers null");

        /* optional outputs guarded */
        CHECK(string_case_substring_find(hay, "WORLD", NULL, NULL)
                  == hay + 6, "the null outputs ride");

        /* corpus gate: empty haystack or empty needle answers null */
        char empty[] = "";
        CHECK(string_case_substring_find(empty, "x", &idx, &len) == NULL
                  && string_case_substring_find(hay, "", &idx, &len) == NULL,
              "the gate answers null");

        /* pointer guards */
        CHECK(string_case_substring_find(NULL, "x", &idx, &len) == NULL
                  && string_case_substring_find(hay, NULL, &idx, &len)
                        == NULL, "the pointer guards hold");

        /* string_folded_compare: ASCII fold over the bound, NUL-stop */
        CHECK(string_folded_compare("AbC", "aBc", 3u) == 0,
              "the fold compare agrees");
        CHECK(string_folded_compare("abc", "abd", 3u) < 0,
              "the fold compare orders low");
        CHECK(string_folded_compare("abd", "abc", 3u) > 0,
              "the fold compare orders high");
        CHECK(string_folded_compare("ab\0", "ab", 3u) == 0,
              "the paired NUL stops at zero");
        CHECK(string_folded_compare("aB", "ab", 99u) == 0,
              "the paired NUL stops before the bound");
    }

    if (s_failures != 0) {
        fprintf(stderr, "string_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("string_test: OK\n");
    return 0;
}
