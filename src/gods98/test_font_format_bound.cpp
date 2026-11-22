// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_font_format_bound — Font.c's font_v_print_f2 (0x0047a1e0, issue #967)
 * copies the caller's format into a 1024-byte buffer, turning each "%b" into
 * an image token. The format can come from game data, so the copy stops at
 * 1023 characters, never starts a "%b" that does not fit whole, and the
 * conversion a cut leaves unfinished reads no argument.
 *
 * The test measures text (render = 0) in a font whose every glyph is
 * GLYPH_WIDTH wide, so a line's width counts the characters that survive the
 * cut. Run under AddressSanitizer or UBSan, an overrun of the copy fails it. */
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "gods98/font_record.h"

extern "C" unsigned int font_v_print_f2(Font* font, int x, int y, int render,
                                        unsigned int* lineCount, const char* msg,
                                        va_list args);

/* "%b"'s image token as Font.c writes it (its FONT_IMAGEFORMAT): the copy
 * holds FMT_CHARS for each "%b", and the formatted line TOKEN_CHARS. */
#if UINTPTR_MAX > 0xffffffffu
#define IMAGE_FORMAT "@[0x%016" PRIxPTR "]"
static const unsigned int TOKEN_CHARS = 16 + 5;
#else
#define IMAGE_FORMAT "@[0x%08" PRIxPTR "]"
static const unsigned int TOKEN_CHARS = 8 + 5;
#endif
static const unsigned int FMT_CHARS = (unsigned int)sizeof(IMAGE_FORMAT) - 1;
static const unsigned int GLYPH_WIDTH = 5;
static const unsigned int COPY_CHARS = 1023; /* sizeof(msgCopy) - 1 */
static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static Font s_font;

static unsigned int measure(unsigned int* lines, const char* msg, ...)
{
    va_list args;
    unsigned int width;

    va_start(args, msg);
    width = font_v_print_f2(&s_font, 0, 0, 0, lines, msg, args);
    va_end(args);
    return width;
}

static void expect(const std::string& format, unsigned int chars, unsigned int lines,
                   const char* what)
{
    unsigned int gotLines = 0;
    unsigned int width = measure(&gotLines, format.c_str());
    if (width != chars * GLYPH_WIDTH || gotLines != lines) {
        printf("  %s: width %u (want %u), lines %u (want %u)\n", what, width,
               chars * GLYPH_WIDTH, gotLines, lines);
        CHECK(0, what);
    }
}

#define ARG0 (uintptr_t)0
#define TEN(x) x, x, x, x, x, x, x, x, x, x

int main()
{
    unsigned int lines = 0;
    std::string f;
    int x, y;

    memset(&s_font, 0, sizeof(s_font));
    for (x = 0; x < FONT_GRIDWIDTH; x++) {
        for (y = 0; y < FONT_GRIDHEIGHT; y++) {
            s_font.posSet[x][y].width = (float)GLYPH_WIDTH;
            s_font.posSet[x][y].height = 10.0f;
        }
    }
    s_font.fontHeight = 10;
    s_font.tabWidth = 40;

    /* Text that fits is unchanged, up to the last character the copy holds. */
    CHECK(measure(&lines, "ab\ncd") == 2 * GLYPH_WIDTH && lines == 2, "a short format measures as before");
    CHECK(measure(&lines, "x%dy", 12) == 4 * GLYPH_WIDTH && lines == 1, "a conversion still takes its argument");
    expect(std::string(COPY_CHARS - 1, 'a'), COPY_CHARS - 1, 1, "1022 characters");
    expect(std::string(COPY_CHARS, 'a'), COPY_CHARS, 1, "1023 characters fit");

    /* A longer format is cut at 1023 characters. */
    expect(std::string(COPY_CHARS + 1, 'a'), COPY_CHARS, 1, "1024 characters");
    expect(std::string(3000, 'a'), COPY_CHARS, 1, "3000 characters");
    expect(std::string(100000, 'a'), COPY_CHARS, 1, "100000 characters");
    expect("x\n" + std::string(3000, 'a'), COPY_CHARS - 2, 2, "a newline, then a long line");
    expect(std::string(1500, 'a') + "\nbb", COPY_CHARS, 1, "a newline past the cut is not seen");

    /* Each "%b" is 2 characters in the format, FMT_CHARS in the copy and
     * TOKEN_CHARS in the formatted line, which holds 1023. Ten of them and
     * 1000 characters fill the copy; the line then cuts what it formats to
     * 1023: ten tokens and the rest in characters. The images are NULL, so a
     * token is 0 wide. */
    f.clear();
    for (x = 0; x < 10; x++) f += "%b";
    f += std::string(1000, 'a');
    lines = 0;
    CHECK(measure(&lines, f.c_str(), TEN(ARG0)) == (COPY_CHARS - 10 * TOKEN_CHARS) * GLYPH_WIDTH &&
              lines == 1,
          "ten %b and a long line");

    /* A thousand "%b" fill the copy by themselves: the tokens that fit take
     * that many of the 130 arguments (113 on a 32-bit host). The line cuts their formatted
     * characters to 1023: whole tokens, and the characters of the next one,
     * which draw as glyphs. */
    f.clear();
    for (x = 0; x < 1000; x++) f += "%b";
    CHECK(COPY_CHARS / FMT_CHARS <= 130, "the copy's tokens have arguments");
    lines = 0;
    CHECK(measure(&lines, f.c_str(), TEN(ARG0), TEN(ARG0), TEN(ARG0), TEN(ARG0), TEN(ARG0),
                  TEN(ARG0), TEN(ARG0), TEN(ARG0), TEN(ARG0), TEN(ARG0), TEN(ARG0), TEN(ARG0),
                  TEN(ARG0)) ==
                  (COPY_CHARS % TOKEN_CHARS) * GLYPH_WIDTH &&
              lines == 1,
          "a thousand %b");

    /* A "%b" that does not fit whole is not started. */
    expect(std::string(COPY_CHARS - FMT_CHARS + 1, 'a') + "%bzz", COPY_CHARS - FMT_CHARS + 1, 1,
           "a %b one character too long");
    expect(std::string(COPY_CHARS - 1, 'a') + "%bzz", COPY_CHARS - 1, 1, "a %b at the last character");
    /* One that fits exactly is expanded; the line then cuts its token. */
    expect(std::string(COPY_CHARS - FMT_CHARS, 'a') + "%bzz", COPY_CHARS, 1, "a %b that fits exactly");

    /* A conversion the cut leaves unfinished takes no argument and prints
     * nothing. */
    {
        std::string c = std::string(1022, 'a') + "%d";
        unsigned int w = measure(&lines, c.c_str(), 42);
        CHECK(w == 1022 * GLYPH_WIDTH && lines == 1, "a cut %d prints nothing");
        c = std::string(1021, 'a') + "%5d";
        w = measure(&lines, c.c_str(), 42);
        CHECK(w == 1021 * GLYPH_WIDTH && lines == 1, "a cut %5d prints nothing");
        c = std::string(1021, 'a') + "%lu";
        w = measure(&lines, c.c_str(), 42);
        CHECK(w == 1021 * GLYPH_WIDTH && lines == 1, "a cut %lu prints nothing");
    }

    if (s_failures) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("font format bound: ok\n");
    return 0;
}
