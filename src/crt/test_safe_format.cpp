// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_safe_format — the units' snprintf and vsnprintf (crt/vc5names.h,
 * crt/safe_format.h) are safe for format strings from game data, and
 * print the same on every host:
 *
 * - standard conversions follow C99;
 * - %n writes nothing;
 * - an unknown letter, %b among them, prints as the letter and takes no
 *   argument, while a "*" before it still takes its int;
 * - an unfinished conversion at the end prints and takes nothing;
 * - widths cap at 1024 and precisions at 256;
 * - the output is bounded and the return value is C99's.
 *
 * The expected strings are written by hand. */
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cmath>

#include "crt/vc5names.h"

static int s_failures = 0;

static char s_formatted[2048];

/* vsnprintf as a unit calls it. */
static int format(const char* msg, ...)
{
    va_list args;
    int len;

    va_start(args, msg);
    len = vsnprintf(s_formatted, sizeof(s_formatted), msg, args);
    va_end(args);
    return len;
}

static void expect(const char* what, int len, const char* expected)
{
    if (len != (int)strlen(expected) || strcmp(s_formatted, expected) != 0) {
        printf("FAIL %s: got \"%s\" (%d), expected \"%s\" (%d)\n", what, s_formatted, len, expected,
               (int)strlen(expected));
        ++s_failures;
    }
}

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

struct IntRow {
    const char* format;
    int args[4];
    const char* expected;
};

/* Formats that take ints, and formats that take nothing. */
static const IntRow kIntRows[] = {
    {"plain text", {0}, "plain text"},
    {"%d|%i|%u|%x", {-42, -42, 42, 255}, "-42|-42|42|ff"},
    {"%X|%o|%o", {255, 8, 0}, "FF|10|0"},
    {"%5d|%-5d|%05d|%+d", {42, 42, 42, 42}, "   42|42   |00042|+42"},
    {"% d|%+05d|%-+5d|%05d", {42, -7, 7, -7}, " 42|-0007|+7   |-0007"},
    {"%.3d|%8.3d|%08.3d|%.0d", {7, 7, 7, 0}, "007|     007|     007|"},
    {"%#x|%#X|%#o|%#08x", {255, 255, 8, 255}, "0xff|0XFF|010|0x0000ff"},
    {"%u|%x", {-1, -1}, "4294967295|ffffffff"},
    {"%hd|%hu|%hhd|%hhu", {70000, -1, 300, -1}, "4464|65535|44|255"},
    {"%c|%3c|%-3c|", {'A', 'B', 'C'}, "A|  B|C  |"},
    {"100%%|%%d", {0}, "100%|%d"},
    {"%5%|%-%", {0}, "%|%"},
    /* Unknown letters print as the letter and take no argument. */
    {"%k%m%y%B%D%U%O%H%r%v%_%!", {1, 2, 3}, "kmyBDUOHrv_!"},
    {"%5k|%-3m|%07.2y|%d", {9}, "k|m|y|9"},
    {"%lk|%zq|%hhw|%d", {9}, "k|q|w|9"},
    {"%I64d|%d", {5, 6}, "I64d|5"},
    /* %b, the port's image token. */
    {"a%bc", {7}, "abc"},
    {"%b %d", {5}, "b 5"},
    {"%-08.3b|%lb|%hb|%d", {4}, "b|b|b|4"},
    /* A "*" takes its int whatever the letter. */
    {"%*b%d", {5, 7}, "b7"},
    {"%.*b%d", {5, 7}, "b7"},
    {"%*.*b%d", {5, 6, 7}, "b7"},
    {"%*k%d", {5, 7}, "k7"},
    {"*%*d*%-*d*", {4, 1, -4, 2}, "*   1*2   *"},
    {"*%*d*", {-3, 1}, "*1  *"},
    {"%.*d|%.*d", {3, 5, -1, 5}, "005|5"},
    /* An unfinished "%" at the end prints nothing. */
    {"100%", {0}, "100"},
    {"50%-5", {0}, "50"},
    {"x%l", {0}, "x"},
    {"y%5.*", {3}, "y"},
    {"z%*", {3}, "z"},
};

struct DoubleRow {
    const char* format;
    double value;
    const char* expected;
};

/* Formats that take doubles, each passed four times. */
static const DoubleRow kDoubleRows[] = {
    {"%f|%.2f|%.0f|%8.3f", 3.14159, "3.141590|3.14|3|   3.142"},
    {"%-8.1f|%08.2f|%+.1f|% .1f", -2.5, "-2.5    |-0002.50|-2.5|-2.5"},
    {"%+08.2f|% .1f|%#.0f|%F", 2.71828, "+0002.72| 2.7|3.|2.718280"},
    {"%e|%.2E|%.0e|%12.3e", 12345.678, "1.234568e+04|1.23E+04|1e+04|   1.235e+04"},
    {"%e|%g|%G|%.3g", 1e-5, "1.000000e-05|1e-05|1E-05|1e-05"},
    {"%e|%g|%g|%G", 1e100, "1.000000e+100|1e+100|1e+100|1E+100"},
    {"%g|%g|%#g|%.10g", 100000.0, "100000|100000|100000.|100000"},
    {"%g|%g|%G|%g", 1000000.0, "1e+06|1e+06|1E+06|1e+06"},
    {"%010.3e|%-12e|%+e|%e", 0.5, "05.000e-01|5.000000e-01|+5.000000e-01|5.000000e-01"},
    {"%lf|%Lf|%hf|%llf", 1.5, "1.500000|1.500000|1.500000|1.500000"},
    {"Game Speed %0.0f%%", 150.0, "Game Speed 150%"},
};

struct StringRow {
    const char* format;
    const char* value;
    const char* expected;
};

/* Formats that take strings, each passed four times. */
static const StringRow kStringRows[] = {
    {"%s|%5s|%-5s|%.2s", "abc", "abc|  abc|abc  |ab"},
    {"%05s|%.0s|%ls|%hs", "abc", "  abc||abc|abc"},
    {"[%s]|[%.3s]|[%8s]", NULL, "[(null)]|[(nu]|[  (null)]"},
    {"[%10s]", "", "[          ]"},
    {"%%s %s %%", "x", "%s x %"},
};

int main()
{
    size_t i;

    for (i = 0; i < sizeof(kIntRows) / sizeof(kIntRows[0]); i++) {
        const IntRow* r = &kIntRows[i];
        expect(r->format, format(r->format, r->args[0], r->args[1], r->args[2], r->args[3]), r->expected);
    }
    for (i = 0; i < sizeof(kDoubleRows) / sizeof(kDoubleRows[0]); i++) {
        const DoubleRow* r = &kDoubleRows[i];
        double v = r->value;
        expect(r->format, format(r->format, v, v, v, v), r->expected);
    }
    for (i = 0; i < sizeof(kStringRows) / sizeof(kStringRows[0]); i++) {
        const StringRow* r = &kStringRows[i];
        const char* v = r->value;
        expect(r->format, format(r->format, v, v, v, v), r->expected);
    }

    /* Length modifiers read the argument at its C type. */
    expect("%ld %lu", format("%ld %lu", -5L, 4000000000UL), "-5 4000000000");
    expect("%lld %llu %llx", format("%lld %llu %llx", -9000000000LL, 18446744073709551615ULL, 0x123456789abcULL),
           "-9000000000 18446744073709551615 123456789abc");
    expect("%zu %zd %zx", format("%zu %zd %zx", (size_t)4000000000u, (size_t)-3, (size_t)255), "4000000000 -3 ff");
    expect("%jd %ju", format("%jd %ju", (intmax_t)-9000000000LL, (uintmax_t)9000000000ULL), "-9000000000 9000000000");
    expect("%td %tu", format("%td %tu", (ptrdiff_t)-12, (ptrdiff_t)12), "-12 12");
    expect("%lld then %d", format("%lld then %d", 1LL << 40, 7), "1099511627776 then 7");

    /* An unfinished conversion takes no argument: these read none. */
    expect("50%*", format("50%*"), "50");
    expect("%-*.*l", format("%-*.*l"), "");

    /* %n writes nothing and still takes its argument. */
    {
        int count = -7;
        short shortCount = -7;
        long long longCount = -7;
        expect("%n", format("ab%ncd%hn%lln%d", &count, &shortCount, &longCount, 5), "abcd5");
        CHECK(count == -7 && shortCount == -7 && longCount == -7, "%n writes nothing");
    }

    /* %p: "0x" and lowercase hex on every host. */
    expect("%p", format("%p|%10p|%-8p|", (void*)(uintptr_t)0xabc, (void*)(uintptr_t)0xabc, (void*)0),
           "0xabc|     0xabc|0x0     |");

    /* Infinities and NaNs have C99's spelling on every host. */
    {
        double inf = HUGE_VAL;
        double nan = inf - inf;
        expect("inf", format("%f|%E|%5.1f|%-6g|%06f", inf, -inf, inf, inf, inf), "inf|-INF|  inf|inf   |   inf");
        expect("+inf", format("%+f|% e|%+G", inf, inf, -inf), "+inf| inf|-INF");
        expect("nan", format("%.1f|%F|%f", std::fabs(nan), std::fabs(nan), -std::fabs(nan)), "nan|NAN|-nan");
    }

    /* Widths cap at 1024 and precisions at 256. */
    CHECK(format("%2000000000d", 1) == 1024 && s_formatted[1022] == ' ' && s_formatted[1023] == '1',
          "a width above 1024 is 1024");
    CHECK(format("%*d", INT_MIN, 1) == 1024 && s_formatted[0] == '1', "a * width of INT_MIN is a capped left width");
    CHECK(format("%.999d", 1) == 256 && s_formatted[0] == '0' && s_formatted[255] == '1',
          "an integer precision above 256 is 256");
    CHECK(format("%.*f", 100000, 1.0) == 258 && strncmp(s_formatted, "1.000", 5) == 0,
          "a float precision above 256 is 256");
    CHECK(format("%.300f", 1e308) == 309 + 1 + 256, "the largest float at the capped precision fits");

    /* The output is bounded; the return value is C99's. */
    {
        char small[8];
        CHECK(snprintf(small, sizeof(small), "%s=%d", "value", 12345) == 11 && strcmp(small, "value=1") == 0,
              "snprintf stores size - 1 characters and returns the whole length");
        CHECK(snprintf(small, sizeof(small), "%20s", "x") == 20 && strcmp(small, "       ") == 0,
              "padding is truncated too");
        CHECK(snprintf(small, 0, "%d", 42) == 2, "snprintf with no room returns the length");
        CHECK(snprintf(NULL, 0, "%s%d", "ab", 123) == 5, "snprintf with no buffer returns the length");
        CHECK(snprintf(small, 1, "%d", 42) == 2 && small[0] == '\0', "a one-byte buffer holds the terminator");
    }

    if (s_failures) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("safe vsnprintf: ok\n");
    return 0;
}
