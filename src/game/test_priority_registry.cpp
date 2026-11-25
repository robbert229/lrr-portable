// SPDX-License-Identifier: LGPL-3.0-or-later
/* Configured task priority ordering, cursor state, and reset on independent
 * numeric rows and generated rectangular resources. */
#include <string>
#include "gods98/config.h"
#include "gods98/directdraw.h"
#include "game/aitask.h"
#include "game/panel_unit.h"
#include "game/priorities.h"
#include "game/lego_unit.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

namespace {
constexpr const char* names[] = {"AI_Priority_Construction", "AI_Priority_Repair",
    "AI_Priority_Reinforce", "AI_Priority_Storage"};
int kinds[4];

void check_order(std::initializer_list<unsigned> expected)
{
    int value = 80;
    for (unsigned i : expected) {
        AITask task{};
        aitask_cmd_set_kind(&task, kinds[i]);
        CHECK_EQ(task.priorityValue, value);
        value -= 5;
    }
}

bool config()
{
    std::string text = "Fixture { Grid { Priorities { ";
    for (unsigned i = 0; i < 4; ++i)
        text += std::string(names[i]) + (i % 2 ? " TRUE " : " FALSE ");
    text += "UnknownPriority TRUE } } Alternate { Priorities { AI_Priority_Storage FALSE } } "
            "PriorityImages { ";
    for (const char* name : names)
        text += std::string(name) + " Sample|SFX_NULL:L\\panel.bmp:L\\load.bmp:L\\panel.bmp ";
    text += "UnknownPriority not_used } PrioritiesImagePositions { "
            "1 5,7 2 25,7 3 45,7 4 65,7 } }";
    auto* parsed = block_parse(text.c_str(), text.size());
    if (!CHECK(parsed != nullptr, "parse neutral priority records")) return false;
    priorities_load_images(parsed, "Fixture");
    priorities_load_positions(parsed, "Fixture");
    CHECK_EQ(priorities_load(parsed, "Fixture", "Grid"), 1);
    for (unsigned i = 0; i < 4; ++i) {
        CHECK(aitask_cmd_kind_by_name(names[i], &kinds[i]), "resolve registered category");
        CHECK_EQ(priority_entry_state_get(kinds[i]), i % 2 ? 0 : 1);
    }
    check_order({0, 1, 2, 3});

    auto& panel = panelGlobs.panelTable[PRIORITIES_PANEL];
    const auto savedFlags = panel.flags;
    const auto savedMode = panelGlobs.flags;
    const auto savedPosition = panel.currPos;
    panel.flags = PANEL_FLAG_OUT;
    indexed_pair_set(PRIORITIES_PANEL, 10, 20);
    // A hit inside row 3 sets hover geometry and selection. A held
    // release in move-to-front mode changes the real scheduler values.
    priorities_selection_set(PRIORITY_TYPE_COUNT, 0);
    CHECK_EQ(priorities_handle_cursor(56, 28, 0, 0, 0), 1);
    CHECK_EQ(priorities_handle_cursor(56, 28, 0, 0, 0), 1);
    Area2F hover{};
    priorities_hover_area_get(&hover);
    CHECK_EQ(hover.x, 55.0f);
    CHECK_EQ(hover.y, 27.0f);
    CHECK_EQ(hover.width, 15.0f);
    CHECK_EQ(hover.height, 15.0f);
    CHECK_EQ(priorities_handle_cursor(56, 28, 1, 0, 1), 1);
    unsigned selected = 0, flags = 0;
    priorities_selection_get(&selected, &flags);
    CHECK_EQ(selected, 2u);
    CHECK_EQ(flags & 3, 3u);
    panelGlobs.flags = 2u;
    CHECK_EQ(priorities_handle_cursor(56, 28, 1, 1, 1), 1);
    check_order({2, 0, 1, 3});
    priorities_draw();
    priorities_selection_get(&selected, &flags);
    CHECK_EQ(flags & 3, 0u);
    CHECK_EQ(priorities_handle_cursor(200, 180, 0, 1, 1), 0);
    priorities_selection_get(&selected, &flags);
    CHECK_EQ(selected, unsigned(PRIORITY_TYPE_COUNT));
    panelGlobs.flags = 4u; // adjacent-row mode
    CHECK_EQ(priorities_handle_cursor(36, 28, 1, 0, 1), 1);
    CHECK_EQ(priorities_handle_cursor(36, 28, 1, 1, 1), 1);
    check_order({0, 2, 1, 3});
    priorities_selection_set(2, 2); // pressed image for an enabled row
    priorities_draw();
    priorities_selection_get(&selected, &flags);
    CHECK_EQ(flags & 2, 0u);
    CHECK_EQ(priorities_handle_cursor(200, 180, 0, 1, 1), 0);
    panelGlobs.flags = 0;
    priorities_change_priority(3);
    check_order({0, 2, 1, 3});
    panel.flags = PANEL_FLAG_IN;
    CHECK_EQ(priorities_handle_cursor(16, 28, 1, 0, 1), 0);
    panel.flags = PANEL_FLAG_MOVING | PANEL_FLAG_OUT;
    CHECK_EQ(priorities_handle_cursor(16, 28, 1, 0, 1), 0);
    panel.flags = PANEL_FLAG_OUT;

    // Load another order and verify reset follows the new configuration,
    // not the previous list. Unknown and absent categories are ignored.
    priorities_toggle(0);
    CHECK_EQ(priority_entry_state_get(kinds[0]), 0);
    priorities_reset();
    check_order({0, 1, 2, 3});
    CHECK_EQ(priority_entry_state_get(kinds[0]), 1);
    CHECK_EQ(priorities_load(parsed, "Fixture", "Alternate"), 1);
    check_order({3});
    CHECK_EQ(priority_entry_state_get(kinds[3]), 1);
    priorities_toggle(0);
    CHECK_EQ(priority_entry_state_get(kinds[3]), 0);
    priorities_reset();
    CHECK_EQ(priority_entry_state_get(kinds[3]), 1);
    CHECK_EQ(priorities_load(parsed, "Fixture", "Absent"), 1);
    priorities_reset();
    CHECK_EQ(priorities_handle_cursor(16, 28, 1, 0, 1), 0);

    panel.flags = savedFlags;
    panel.currPos = savedPosition;
    panelGlobs.flags = savedMode;
    block_close(parsed);
    return true;
}
}

int main()
{
    if (!synthetic_fixture::initialise_full_game("priority_registry", "PriorityRegistry"))
        return lrr_test_result();
    config();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("priority_registry");
}
