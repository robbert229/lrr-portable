// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_encyclopedia — selecting a resource without an entry fills the
 * dock once, and removing the selected object clears its ground marker. */
#include <cstdarg>

#include "game/encyclopedia_link_names.h"
#include "game/encyclopedia.h"
#include "game/encyclopedia_imports.h"
#include "game/legoobject_record.h"
#include "test/lrr_test.h"

static std::string dock;
static unsigned int draws;
static Lego_Object* marker;

extern "C" void __wrap_ui_text_output_object_release_target(void* const*)
{
    dock.clear();
}

extern "C" void __wrap_panel_text_window_print_f(PanelTextWindow*, const char* format, ...)
{
    char text[128];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    dock += text;
}

extern "C" void __wrap_ui_text_output_position_and_draw(void*, unsigned int, float)
{
    ++draws;
}

extern "C" void __wrap_lego_selected_entity_ground_marker_render(
    Lego_Object* object, ViewProjectionAdapter*, float, float, float)
{
    marker = object;
}

int main(void)
{
    Lego_Object crystal = {}, other = {};
    crystal.type = 6; // Power crystal.
    dock = "previous selection";
    panelGlobs.panelTable[PANEL_ENCYCLOPEDIA].flags = PANEL_FLAG_IN;
    encyclopedia_select_object(&crystal);
    CHECK(dock.empty(), "selection clears the old entry");
    encyclopedia_update(1.0f);
    CHECK_EQ(draws, 0u);
    CHECK(dock.empty(), "a closed dock does not consume the pending update");

    panelGlobs.panelTable[PANEL_ENCYCLOPEDIA].flags = PANEL_FLAG_OUT;
    encyclopedia_update(1.0f);
    encyclopedia_update(1.0f);
    CHECK_EQ(draws, 2u);
    CHECK_STREQ(dock.c_str(), "Object has no encyclopedia file.");

    encyclopedia_remove_current_reference(&other);
    encyclopedia_draw_select_box(nullptr);
    CHECK_EQ(marker, &crystal);
    encyclopedia_remove_current_reference(&crystal);
    marker = nullptr;
    encyclopedia_draw_select_box(nullptr);
    CHECK_EQ(marker, nullptr);

    encyclopedia_disable_update();
    encyclopedia_update(1.0f);
    CHECK_EQ(draws, 2u);
    return lrr_test_result("encyclopedia");
}
