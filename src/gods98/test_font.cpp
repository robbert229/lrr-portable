// SPDX-License-Identifier: LGPL-3.0-or-later
/* font.c — variable glyph advances, tabs, unsupported bytes and multiline
 * measurement on a synthetic glyph grid without rendering. */
#include <cstdarg>

#include "gods98/font_link_names.h"
#include "gods98/font_record.h"
#include "test/lrr_test.h"

extern "C" unsigned int font_get_char_width(Font* font, unsigned char c);
extern "C" unsigned int font_get_height(Font* font);
extern "C" unsigned int font_v_print_f2(Font* font, int x, int y, int render,
                                        unsigned int* lineCount, const char* msg, va_list args);

static unsigned int measure(Font* font, unsigned int* lines, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    unsigned int width = font_v_print_f2(font, 0, 0, 0, lines, format, args);
    va_end(args);
    return width;
}

int main()
{
    Font font = {};
    font.fontHeight = 12;
    font.tabWidth = 10;
    font.posSet[0][0].width = 2.0f;  // Space (32).
    font.posSet[3][3].width = 3.0f;  // A (65).
    font.posSet[4][3].width = 7.0f;  // B (66).
    font.posSet[9][18].width = 5.0f; // Last slot (221).

    CHECK_EQ(font_get_height(&font), 12u);
    CHECK_EQ(font_get_char_width(&font, ' '), 2u);
    CHECK_EQ(font_get_char_width(&font, 'A'), 3u);
    CHECK_EQ(font_get_char_width(&font, 'B'), 7u);
    CHECK_EQ(font_get_char_width(&font, 221), 5u);
    CHECK_EQ(font_get_char_width(&font, 222), 0u);
    CHECK_EQ(font_get_char_width(&font, 31), 0u);

    unsigned int lines = 0;
    CHECK_EQ(measure(&font, &lines, "A\tB\nB\n"), 17u);
    CHECK_EQ(lines, 3u);
    CHECK_EQ(measure(&font, &lines, "AB\tA"), 23u);
    CHECK_EQ(lines, 1u);
    CHECK_EQ(measure(&font, &lines, ""), 0u);
    CHECK_EQ(lines, 1u);
    CHECK_EQ(measure(&font, nullptr, "%s", "A B"), 12u);
    return lrr_test_result();
}
