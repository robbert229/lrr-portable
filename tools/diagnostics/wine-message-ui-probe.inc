// SPDX-License-Identifier: LGPL-3.0-or-later
/* Opt-in disposable PE32 production-owner probe. NOT BUILT OR RUN.
 * Keep outside all Reccmp/source scan roots. Invoke only with real frontend
 * resources, active render target and loaded M01 tokens. It deliberately
 * replaces requests and exercises frontend ownership; never use this process
 * as ordinary authored-script gameplay acceptance. */
#if !defined(_WIN32) || !defined(_M_IX86)
#error This probe requires the MSVC5 PE32 candidate
#endif
#include <stdio.h>
#include <string.h>
#include "crt/crt.hpp"
#include "game/frontend.hpp"
#include "game/objective.hpp"
#include "game/tutorial_hud_ports.hpp"
#include "game/util.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/surface.hpp"
#include "gods98/texture.hpp"

static float message_probe_float(const void* record, unsigned int offset)
{
    float value;
    memcpy(&value, (const char*)record + offset, sizeof(value));
    return value;
}

static int message_probe_pixels(unsigned long* hash_out)
{
    DisplaySurface* surface = surface_active_render_target_get();
    void* pixels = NULL;
    int pitch = 0;
    int y, x;
    unsigned long hash = 2166136261UL;
    if (surface == NULL || surface->h <= 0 ||
        surface_lock(surface, &pixels, &pitch) != 0) return 0;
    if (pixels == NULL || pitch <= 0) {
        surface_unlock(surface, pixels);
        return 0;
    }
    for (y = 0; y < surface->h; ++y) {
        const unsigned char* row = (const unsigned char*)pixels + y * pitch;
        for (x = 0; x < pitch; ++x) {
            hash ^= row[x];
            hash *= 16777619UL;
        }
    }
    surface_unlock(surface, pixels);
    *hash_out = hash;
    return 1;
}

/* 0 success, 1 observed contract failure, 2 missing prerequisite/inconclusive.
 * Uses the production wrapper's own overflow result; no alternate wrap model
 * or guessed glyph width is an expected value. */
int pe32_message_ui_review_probe(BlockFile* cfg, int width, int height, FILE* log)
{
    void* record = ui_message_text_output_get(0);
    const char* long_text;
    const char* short_text;
    void* font;
    int line_height, frame, bottom = 0, overflow = 1;
    float initial_height, expanded_height;
    float space_rect[4];
    char key[256];
    const char* path;
    const char* position;
    TextureSurface* image;
    float button_x, button_y;
    int x, y;
    AdvisorMessagePointerCapture capture;
    unsigned long before, after;
    if (sizeof(void*) != 4 || cfg == NULL || log == NULL || record == NULL ||
        width <= 0 || height <= 0) return 2;
    tutorial_message_generation_sync();
    long_text = objective_advisor_token_text_get(0u);
    short_text = objective_advisor_token_text_get(22u);
    if (long_text == NULL || short_text == NULL) return 2;
    memcpy(&font, record, sizeof(font));
    line_height = font_atlas_record_line_height_get(font);
    if (line_height <= 0 ||
        !font_atlas_record_glyph_get(font, ' ', space_rect)) return 2;
    fprintf(log, "production space advance=%g (reference minimum1)\n",
            space_rect[2]);
    if (space_rect[2] < 1.0f) return 1;
    fprintf(log, "fontLine=%d longBytes=%u shortBytes=%u\n", line_height,
            (unsigned int)strlen(long_text), (unsigned int)strlen(short_text));
    advisor_message_ui_gameplay_set(1);
    initial_height = message_probe_float(record, 0x10u);
    if (initial_height != 42.0f ||
        message_probe_float(record, 8u) != 435.0f) {
        fprintf(log, "INCONCLUSIVE: invoke before first message UI draw, "
                     "with initial production widget geometry\n");
        return 2;
    }
    /* Config/image determine the real control point; no handpicked screen
     * coordinate, fabricated bitmap or forced NEXT-enable bit. */
    snprintf(key, sizeof(key), "%s Main NextButton%ix%i", util_root_name(cfg),
             width, height);
    path = block_get_strptr(cfg, key);
    snprintf(key, sizeof(key), "%s Main NextButtonPos%ix%i", util_root_name(cfg),
             width, height);
    position = block_get_strptr(cfg, key);
    if (path == NULL || position == NULL ||
        sscanf(position, "%f,%f", &button_x, &button_y) != 2 ||
        (engine_debug_word_peek() & 0x40u) == 0u) return 2;
    image = image_surface_load(path, 0u, 0u);
    if (image == NULL) return 2;
    x = (int)(button_x + (float)image->width * 0.5f);
    y = (int)(button_y + (float)image->height * 0.5f);
    texture_release(image);

    advisor_text_request_setup_text(long_text, 0u);
    for (frame = 0; frame < 32; ++frame) {
        advisor_message_ui_draw(0.0f);
        overflow = text_buffer_draw_wrapped(record, 0u, 0.0f, &bottom);
        fprintf(log, "long frame=%d y=%g height=%g bottom=%d overflow=%d\n",
                frame, message_probe_float(record, 8u),
                message_probe_float(record, 0x10u), bottom, overflow);
        if (!overflow) break;
    }
    expanded_height = message_probe_float(record, 0x10u);
    if (overflow || expanded_height <= initial_height ||
        message_probe_float(record, 8u) <= 0.0f ||
        bottom + line_height > height) return 1;
    /* Present the settled expanded placement, then test the actual loaded
     * NEXT control before contraction can hide an expanded-panel hit bug. */
    advisor_message_ui_draw(0.0f);
    memset(&capture, 0, sizeof(capture));
    if (!advisor_message_pointer_capture_dispatch(&capture, x, y, 1) ||
        capture.owner != 1) {
        fprintf(log, "FAIL NEXT does not acquire on expanded long message\n");
        return 1;
    }
    fprintf(log, "expanded NEXT acquired x=%d y=%d height=%g\n",
            x, y, message_probe_float(record, 0x10u));
    /* Retire the captured DOWN before UP, with no NEXT acknowledgement. */
    advisor_message_ui_gameplay_set(0);
    advisor_message_pointer_capture_dispatch(&capture, x, y, 1);
    advisor_message_pointer_capture_dispatch(&capture, x, y, 0);
    if (capture.owner != 0 || capture.down) return 1;
    advisor_message_ui_gameplay_set(1);
    advisor_text_request_setup_text(short_text, 0u);
    for (frame = 0; frame < 32; ++frame) {
        advisor_message_ui_draw(0.0f);
        if (message_probe_float(record, 0x10u) == initial_height) break;
    }
    overflow = text_buffer_draw_wrapped(record, 0u, 0.0f, &bottom);
    fprintf(log, "short y=%g height=%g bottom=%d overflow=%d\n",
            message_probe_float(record, 8u),
            message_probe_float(record, 0x10u), bottom, overflow);
    if (overflow || message_probe_float(record, 0x10u) != initial_height ||
        message_probe_float(record, 8u) != 435.0f) return 1;

    memset(&capture, 0, sizeof(capture));
    if (!advisor_message_pointer_capture_dispatch(&capture, x, y, 1) ||
        capture.owner != 1) return 1;
    advisor_message_ui_gameplay_set(0);
    if (!message_probe_pixels(&before)) return 2;
    advisor_message_ui_draw(0.0f);
    if (!message_probe_pixels(&after)) return 2;
    if (before != after) return 1;
    advisor_message_pointer_capture_dispatch(&capture, x, y, 1);
    if (capture.owner != 0 || !capture.down) return 1;
    advisor_message_ui_gameplay_set(1);
    advisor_message_pointer_capture_dispatch(&capture, x, y, 1);
    if (capture.owner != 0) return 1;
    advisor_message_pointer_capture_dispatch(&capture, x, y, 0);
    if (capture.owner != 0 || capture.down) return 1;
    advisor_message_pointer_capture_dispatch(&capture, x, y, 1);
    if (capture.owner != 1) return 1;
    /* Cancel without issuing NEXT: this probe never advances an authored
     * stage as a substitute for ordinary user interactions. */
    advisor_message_ui_gameplay_set(0);
    advisor_message_pointer_capture_dispatch(&capture, x, y, 0);
    fprintf(log, "PASS frontend layout/owner boundary; frame reward-owner "
                 "mapping and ordinary NEXT/gameplay remain UNVERIFIED\n");
    return 0;
}
