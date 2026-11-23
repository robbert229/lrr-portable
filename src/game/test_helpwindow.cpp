// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_helpwindow — enablement, visibility gates, one-time help for an
 * object level, and the same-button rule for a mouse release. */
#include "game/helpwindow_link_names.h"
#include "game/helpwindow.h"
#include "gods98/config.h"
#include "test/lrr_test.h"

static int tutorialFlags;

extern "C" void __wrap_text_buffer_reset_target(void*)
{
}

extern "C" int __wrap_nerpfunc_get_tutorial_flags(unsigned int*)
{
    return tutorialFlags;
}

int main(void)
{
    const char config[] = "Game { HelpWindow { } }";
    BlockFile* root = block_parse(config, sizeof(config) - 1u);
    if (!CHECK(root != nullptr, "the empty help config parses"))
        return lrr_test_result("helpwindow");
    help_window_initialise(root, "Game");
    block_close(root);
    block_text_token_pool_destroy();
    CHECK(help_window_is_enabled() != 0, "help starts enabled");
    CHECK_EQ(help_window_is_showing(), 0);
    help_window_recall_dependencies(4, 0, 0, 0);
    CHECK_EQ(help_window_is_showing(), 1);
    tutorialFlags = 1;
    CHECK_EQ(help_window_is_showing(), 0);
    tutorialFlags = 0;
    help_window_toggle(1, 0);
    CHECK_EQ(help_window_is_enabled(), 0);
    CHECK_EQ(help_window_is_showing(), 0);
    help_window_toggle(0, 0);
    CHECK_EQ(help_window_is_showing(), 1);
    help_window_hide();
    help_window_recall_dependencies(4, 0, 0, 0);
    CHECK_EQ(help_window_is_showing(), 0);
    help_window_recall_dependencies(4, 0, 1, 0);
    CHECK_EQ(help_window_is_showing(), 1);

    help_window_press_button(0, 0, 0); // Start with the mouse released.
    CHECK_EQ(help_window_press_button(0, 1, 1), 0);
    CHECK_EQ(help_window_press_button(0, 0, 1), 0);
    CHECK_EQ(help_window_press_button(0, 1, 0), 0);
    CHECK_EQ(help_window_press_button(1, 1, 1), 0);
    CHECK_EQ(help_window_press_button(0, 0, 1), 1);
    CHECK_EQ(help_window_press_button(0, 0, 0), 0);
    return lrr_test_result("helpwindow");
}
