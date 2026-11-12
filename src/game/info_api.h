// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Info.c (GAME-019, finding 567):
 * the callees in other units that have no C declaration a C unit can
 * include without moving another C unit's symbol counter. Lego.c
 * (gods98/engine.hpp), Images.c (gods98/record.hpp), Pointer.c (game/frontend.hpp, crt/crt.hpp), the
 * information panel widget (crt/crt.hpp) and SFX.c (gods98/sound.hpp).
 * Only info_unit.c and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_INFO_API_H
#define LEGORRR_GAME_INFO_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float EngineTimeScale(void);                                    /* 0041fb40 */
void scroll_scale_set_and_clamp(float scale);                   /* 00425ce0 */
/* Lego_GotoInfo (0042de40) in the original ABI; unannotated adapter
 * (engine.cpp). */
void lego_goto_info(void* object, const int* blockPos, int force);
int pointer_cursor_config_parse_target(const char* name,
                                       int* index_out);         /* 0045cbd0 */
uint32_t indexed_global_value_at_get(int index);                /* 0045cc10 */
void indexed_text_metrics_pair_get(int index, uint32_t* out_x,
                                   uint32_t* out_y);            /* 00463ea0 */
void indexed_text_metrics_pair_set(int index, uint32_t x,
                                   uint32_t y);                 /* 00463ed0 */
/* Image_SetPenZeroTrans (0047dbc0). The port's entry takes its
 * TextureSurface; info_unit.c passes the Image, as the original does. The
 * unit's image setters have no native caller (finding 567). */
void resource_record_add_ref(void* record);                     /* 0047dbc0 */
int sound_submit_with_global_cooldown(int alias, int* out_handle); /* 00464ef0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_INFO_API_H */
