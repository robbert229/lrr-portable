// SPDX-License-Identifier: LGPL-3.0-only
// Corpus text primitives (findings 397/12): duplicate, substring find,
// bounded length, and the `::` config-path builder.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    free(dup);

    char* empty = string_duplicate("");
    CHECK(empty != NULL && empty[0] == '\0', "empty string duplicates to a byte");
    free(empty);

    CHECK(string_duplicate(NULL) == NULL, "NULL duplicates to NULL");
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

static void test_path_join(void)
{
    const char* p = string_path_join("Lego*", "Main Menu", "MenuCount", NULL);
    CHECK(strcmp(p, "Lego*::Main Menu::MenuCount") == 0, "joins with :: runs");
    p = string_path_join("Lego*", NULL);
    CHECK(strcmp(p, "Lego*") == 0, "root-only stays the root");
    p = string_path_join("a", "b", NULL);
    CHECK(strcmp(p, "a::b") == 0, "builder restarts per call");

    /* Over-long root: truncated into the static buffer, never a crash. */
    char filler[STRING_PATH_MAX + 16];
    memset(filler, 'r', sizeof filler);
    filler[sizeof filler - 1] = '\0';
    p = string_path_join(filler, "tail", NULL);
    CHECK(strlen(p) == STRING_PATH_MAX - 1 && strstr(p, "::") == NULL,
          "the full buffer truncates the root and joins nothing further");
}

int main(void)
{
    test_duplicate();
    test_find_substring();
    test_length_bounded();
    test_path_join();
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
        CHECK(case_insensitive_substring_find(NULL, "x") == NULL
                  && case_insensitive_substring_find("abc", NULL) == NULL,
              "the pointer guards hold");
        CHECK(case_insensitive_substring_find("ab", "abc") == NULL,
              "the tail overrun does not match");
    }

    if (s_failures != 0) {
        fprintf(stderr, "string_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("string_test: OK\n");
    return 0;
}
