// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_info_image_token — Info.c's info_compile_text (0x00419530) turns a
 * $name$ token in an information message into "%b" and stores the token's
 * image after the text's terminator; info_draw_panel (0x0041a080) passes
 * those images as the varargs of the panel print, and Font.c's
 * font_v_print_f2 (0x0047a1e0) draws the image a "%b" names.
 *
 * The original stores one byte of each Image pointer and Font reads eight
 * hex digits, so the image that comes back is not the one the token named.
 * The port carries the whole pointer.
 *
 * The test links with --wrap for the image loader, the image draw and the
 * panel calls, so the units reach the stubs below. */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/info_unit.h"
#include "gods98/font_api.h"
#include "gods98/font_record.h"

/* Pointer.c's Pointer_Initialise: the pointer names a token may name. */
extern "C" void ui_pointer_cursor_name_registry_init(void);

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* The image the information type loads, and the images Font draws. */
static Image* s_image;
static Image* s_drawn[8];
static int s_drawnCount;

extern "C" Image* __wrap_Image_LoadBMPScaled(const char*, unsigned int, unsigned int)
{
    return s_image;
}
extern "C" void __wrap_Image_SetPenZeroTrans(void*) {}
extern "C" void __wrap_Image_DisplayScaled(Image* image, const Area2F*, const Point2F*, const Point2F*)
{
    /* Font's glyphs draw from the font's own image, NULL here. */
    if (image && s_drawnCount < 8) s_drawn[s_drawnCount++] = image;
}

/* The panel calls info_draw_panel makes; the print records its format and
 * its first image argument. */
static char s_printed[256];
static uintptr_t s_printedImage;
static int s_printCount;

extern "C" void __wrap_panel_text_window_print_f(void*, const char* msg, ...)
{
    va_list args;

    ++s_printCount;
    snprintf(s_printed, sizeof(s_printed), "%s", msg);
    if (strstr(msg, "%b")) {
        va_start(args, msg);
        s_printedImage = va_arg(args, uintptr_t);
        va_end(args);
    }
}
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
    Font font;
    Image* trailer;
    char* text = NULL;
    size_t len;
    int type;

    /* An image above 4 GiB where the platform allows it (glibc maps a
     * block this large): its low byte and its low 32 bits name nothing. */
    s_image = (Image*)calloc(1, 1 << 20);
    s_image->width = 7;
    s_image->height = 9;

    ui_pointer_cursor_name_registry_init();
    info_initialise(NULL);
    CHECK(info_get_type("Info_CrystalFound", &type), "Info_CrystalFound is a type");
    info_set_image_file(type, "crystal.bmp");
    CHECK(info_get_image(type) == s_image, "the type's image is the loaded image");

    /* The compiled text: "%b" in place of the token, then the count and the
     * whole image pointer. */
    info_compile_text((char*)"Found_$Info_CrystalFound$_here", &text);
    CHECK(strcmp(text, "Found %b here") == 0, "the token becomes %b");
    len = strlen(text);
    CHECK(text[len + 1] == 1, "the trailer counts one image");
    memcpy(&trailer, text + len + 2, sizeof(trailer));
    CHECK(trailer == s_image, "the trailer holds the whole image pointer");

    /* Font draws the image "%b" names. */
    memset(&font, 0, sizeof(font));
    s_drawnCount = 0;
    font_print_f((const TextureSurface*)&font, 0, 0, text, (uintptr_t)s_image);
    CHECK(s_drawnCount == 1, "Font draws one image");
    CHECK(s_drawnCount >= 1 && s_drawn[0] == s_image, "Font draws the token's image");
    s_drawnCount = 0;
    font_print_f((const TextureSurface*)&font, 0, 0, "@[1] @[0x12] @[0x0000000g00000000] @[0xzzzzzzzz] @[");
    CHECK(s_drawnCount == 0, "Font draws no image for other bracketed text");
    free(text);

    /* A token that names no image is dropped; past five images the rest are
     * dropped too, so no "%b" is left without its argument. */
    info_compile_text((char*)"a$Nothing$b", &text);
    CHECK(strcmp(text, "ab") == 0 && text[3] == 0, "an unknown token is dropped");
    free(text);
    info_compile_text((char*)"$Info_CrystalFound$$Info_CrystalFound$$Info_CrystalFound$"
                             "$Info_CrystalFound$$Info_CrystalFound$$Info_CrystalFound$", &text);
    CHECK(strcmp(text, "%b%b%b%b%b") == 0, "five images at most");
    CHECK(text[strlen(text) + 1] == 5, "the trailer counts five images");
    free(text);

    /* The panel: a docked message of the type prints its text with the
     * whole image pointer. */
    info_set_text(type, (char*)"Found_$Info_CrystalFound$_here");
    info_enable(1);
    info_send(type, NULL, NULL, NULL);
    pending_record_list_entry_move_to_front(0);
    s_printCount = 0;
    info_draw_panel(0.0f);
    CHECK(s_printCount == 1, "the panel prints the docked message");
    CHECK(strcmp(s_printed, "Found %b here") == 0, "the panel prints the compiled text");
    CHECK(s_printedImage == (uintptr_t)s_image, "the panel passes the whole image pointer");

    if (s_failures) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("info image token: ok\n");
    return 0;
}
