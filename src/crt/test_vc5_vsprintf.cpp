// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_vc5_vsprintf — the units' vsnprintf (crt/vc5names.h) prints "%b" as
 * the original's VC5 LIBC does: the "b" as text, with no argument taken,
 * whatever flags, width, precision or size come before it. A "%" left
 * unfinished at the end of the format prints nothing. Every other format
 * goes to the host's vsnprintf unchanged. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crt/vc5names.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* vsnprintf as a unit calls it. */
static char s_formatted[256];
static int format(const char* msg, ...)
{
    va_list args;
    int len;

    va_start(args, msg);
    len = vsnprintf(s_formatted, sizeof(s_formatted), msg, args);
    va_end(args);
    return len;
}

int main()
{
    static int image;

    CHECK(format("a%bc", (uintptr_t)&image) == 3 && strcmp(s_formatted, "abc") == 0, "%b prints a b");
    CHECK(format("%b %s", "next") == 6 && strcmp(s_formatted, "b next") == 0, "%b takes no argument");
    CHECK(format("%d%b", 7) == 2 && strcmp(s_formatted, "7b") == 0, "%b after another conversion");
    CHECK(format("%-08.3b|%lb|%I64b|%hb", (uintptr_t)&image) == 7 && strcmp(s_formatted, "b|b|b|b") == 0,
          "%b with flags, width, precision and size");
    CHECK(format("%%b %5.1s%%", "xy") == 9 && strcmp(s_formatted, "%b     x%") == 0, "%%b stays text");
    CHECK(format("100%") == 3 && strcmp(s_formatted, "100") == 0, "a lone % at the end prints nothing");
    CHECK(format("50%-5") == 2 && strcmp(s_formatted, "50") == 0, "an unfinished % at the end prints nothing");
    CHECK(format("%s=%d", "x", 4) == 3 && strcmp(s_formatted, "x=4") == 0, "other formats are the host's");

    if (s_failures) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("vc5 vsnprintf: ok\n");
    return 0;
}
