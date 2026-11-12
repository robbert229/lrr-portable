// SPDX-License-Identifier: LGPL-3.0-or-later
/* The information and message panels (original units GAME-019 and
 * GAME-039) that units rebuilt as C call, with the port's contracts.
 *
 * C boundary header (ADR 0005). The C++ declarations (game/util.hpp,
 * crt/crt.hpp, game/frontend.hpp, gods98/resource.hpp) carry extern "C";
 * src/game/test_c_boundaries.cpp includes them with this header. Shared C++
 * headers do not include this file (finding 506). */
#ifndef LEGORRR_GAME_INFO_H
#define LEGORRR_GAME_INFO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00419da0, FUN_00419de0, FUN_00419e60, FUN_0041a290 (GAME-019,
 * Info.c): the message list and the scroll position. */
void incident_message_panel_refresh_if_active(void);
void pending_record_list_entry_move_to_front(unsigned int index);
void pending_record_release_if_present(void);
void screen_right_edge_cache_refresh(void);

/* FUN_004636b0 (GAME-039): the information panel's scroll widget. The port
 * has no entry at this address; thin unannotated adapter (ADR 0005 step 6)
 * in info_track_overlay.cpp. */
void info_panel_initialise(const char* name, int a, int b, int panel,
                           int c);
/* FUN_00463730, FUN_00463770, FUN_00463810 (GAME-039). */
void indexed_float_pair_set(int index, uint32_t x, uint32_t y);
void scroll_rect_resource_record_create(int view, int mode,
                                        const float* quad, int tex_a,
                                        int tex_b);
void scroll_rect_payload_record_create(int slot, const float rect[4],
                                       int count, uint32_t value,
                                       const char* resource_name);
/* FUN_004638a0 (GAME-039): the mouse over the scroll widget. Thin
 * unannotated adapter in info_track_overlay.cpp. */
int info_panel_check_collision(int* released, int mouseX, int mouseY,
                               int leftButton, int leftButtonLast);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_INFO_H */
