// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_info_panel_percent_b — Info.c's info_compile_text (0x00419530) puts
 * "%b" in place of each $name$ image token, and info_draw_panel (0x0041a080)
 * prints the text into the info panel's TextWindow through
 * panel_text_window_print_f (0x0045a590) and TextWindow_VPrintF
 * (0x00485250), which format with the C library's vsprintf.
 *
 * The original's VC5 LIBC has no "b" conversion: it prints the "b" as text
 * and takes no argument. glibc prints the argument in binary and the UCRT
 * aborts, so the units' vsnprintf (crt/vc5names.h) handles "%b" itself and
 * the panel shows the same text on every host. test_safe_format checks
 * that vsnprintf on its own.
 *
 * The test links with --wrap for the image loader, the image draw and the
 * panel calls other than the print, so the units reach the stubs below. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/info_unit.h"
#include "game/panel_unit.h"
#include "gods98/textwindow_record.h"

/* Pointer.c's Pointer_Initialise: the pointer names a token may name. */
extern "C" void ui_pointer_cursor_name_registry_init(void);
/* TextWindow_PrintOverlay (0x00485300). */
extern "C" void objective_advisor_text_copy(void* window, int oneFrame, const char* msg, ...);

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static Image* s_image;

extern "C" Image* __wrap_Image_LoadBMPScaled(const char*, unsigned int, unsigned int)
{
    return s_image;
}
extern "C" void __wrap_Image_SetPenZeroTrans(void*) {}
extern "C" void __wrap_Image_DisplayScaled(Image*, const Area2F*, const Point2F*, const Point2F*) {}
extern "C" void __wrap_panel_state_flags_toggle(int) {}
extern "C" void __wrap_ui_text_output_object_release_target(void* const*) {}
extern "C" void __wrap_ui_text_output_position_and_draw(void*, unsigned int, float) {}
extern "C" void __wrap_ui_record_callback_forward(const void* const*, uint32_t* lines, uint32_t* shown)
{
    *lines = *shown = 0;
}
extern "C" void __wrap_indexed_text_metrics_pair_set(int, uint32_t, uint32_t) {}
extern "C" void __wrap_indexed_text_metrics_pair_get(int, uint32_t* x, uint32_t* y)
{
    *x = *y = 0;
}
extern "C" int __wrap_sound_submit_with_global_cooldown(int, int*) { return 0; }

int main()
{
    static char ring[TEXTWINDOW_MAXSTRINGLEN];
    TextWindow window;
    PanelTextWindow panelWindow;
    int type;

    s_image = (Image*)calloc(1, sizeof(Image) + 64);

    /* The info panel's TextWindow. */
    memset(&window, 0, sizeof(window));
    window.windowBuf = ring;
    window.windowBufLen = sizeof(ring);
    memset(&panelWindow, 0, sizeof(panelWindow));
    panelWindow.textWindow = &window;
    panelGlobs.scrollTextWindow = &panelWindow;

    ui_pointer_cursor_name_registry_init();
    info_initialise(NULL);
    CHECK(info_get_type("Info_CrystalFound", &type), "Info_CrystalFound is a type");
    info_set_image_file(type, "crystal.bmp");
    info_set_text(type, (char*)"Found_$Info_CrystalFound$_and_$Info_CrystalFound$_here");
    info_enable(1);
    info_send(type, NULL, NULL, NULL);
    pending_record_list_entry_move_to_front(0);
    info_draw_panel(0.0f);
    CHECK(window.usedLen == strlen("Found b and b here") &&
              memcmp(window.windowBuf, "Found b and b here", window.usedLen) == 0,
          "the panel buffers each token as a b");

    /* The overlay print formats the same way. */
    objective_advisor_text_copy(&window, 1, "Found %b here", (uintptr_t)s_image);
    CHECK(strcmp(window.secondaryBuf, "Found b here") == 0, "the overlay prints a b");

    if (s_failures) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("info panel %%b: ok\n");
    return 0;
}
