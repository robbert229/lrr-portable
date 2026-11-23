// SPDX-License-Identifier: LGPL-3.0-or-later
/* textwindow.c — formatted ring storage, page selection and a timed
 * overlay. Collect the displayed glyphs and their positions in memory. */
#include <string>
#include <vector>

#include "gods98/textwindow_link_names.h"
#include "gods98/textwindow_record.h"
#include "test/lrr_test.h"

extern "C" TextWindow* text_window_create(Font*, TextWindowArea*, unsigned int);
extern "C" void text_window_remove(TextWindow*);
extern "C" void text_window_printf(TextWindow*, const char*, ...);
extern "C" void text_window_page_printf(TextWindow*, unsigned int, const char*, ...);
extern "C" void text_window_clear(TextWindow*);
extern "C" void text_window_printf_overlay(TextWindow*, int, const char*, ...);
extern "C" int text_window_update(TextWindow*, unsigned int, float, int*);

struct Glyph {
    char c;
    int x;
    int y;
};
static std::vector<Glyph> glyphs;

extern "C" unsigned int __wrap_font_output_char(Font*, int x, int y, unsigned char c, int render)
{
    if (c < 32) return 0;
    if (render) glyphs.push_back({static_cast<char>(c), x, y});
    return 5;
}

static std::string displayed()
{
    std::string text;
    for (const auto& glyph : glyphs) text += glyph.c;
    return text;
}

int main()
{
    Font font = {};
    font.fontHeight = 10;
    for (auto& col : font.posSet)
        for (auto& glyph : col) glyph.width = 5;
    TextWindowArea area = {10, 20, 30, 30};
    TextWindow* window = text_window_create(&font, &area, 8);
    if (!CHECK(window != nullptr && window->windowBuf != nullptr, "allocate text ring")) {
        if (window) text_window_remove(window);
        return lrr_test_result();
    }

    text_window_printf(window, "%s", "abc");
    text_window_printf(window, "%s", "DEFGHI");
    CHECK_EQ(window->usedLen, 1u);
    CHECK_EQ(std::memcmp(window->windowBuf, "IbcDEFGH", 8), 0);

    text_window_clear(window);
    CHECK_EQ(window->usedLen, 0u);
    CHECK_EQ(window->lineCount, 0u);
    text_window_page_printf(window, 2, "only\aone-break");
    CHECK_EQ(window->usedLen, 0u);
    text_window_page_printf(window, 1, "one\a%s", "TWO");
    CHECK_EQ(window->usedLen, 3u);

    text_window_printf_overlay(window, 0, "%s", "XY");
    int height = -1;
    CHECK_EQ(text_window_update(window, 0, 1.0f, &height), 0);
    CHECK_EQ(displayed(), std::string("XY"));
    CHECK_EQ(height, 20);
    if (CHECK_EQ(glyphs.size(), std::size_t{2})) {
        CHECK_EQ(glyphs[0].x, 20);
        CHECK_EQ(glyphs[1].x, 25);
        CHECK_EQ(glyphs[0].y, 20);
    }

    glyphs.clear();
    CHECK_EQ(text_window_update(window, 0, 25.0f, nullptr), 0);
    CHECK_EQ(displayed(), std::string("XY"));
    glyphs.clear();
    CHECK_EQ(text_window_update(window, 0, 1.0f, &height), 0);
    CHECK_EQ(displayed(), std::string("TWO"));
    CHECK_EQ(height, 30);
    CHECK_EQ(window->lineCount, 1u);
    CHECK_EQ(window->maxLines, 3u);
    text_window_remove(window);
    return lrr_test_result();
}
