// SPDX-License-Identifier: LGPL-3.0-or-later
/* Bubbles.c (original translation unit GAME-005, finding 505; finding 598):
 * the minifigures' thought bubbles, the health bars of damaged objects and
 * the power-off icons of unpowered buildings, by the unit's own names.
 *
 * C-compatible header (ADR 0005). bubbles.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it.
 *
 * In the matching build bubbles_link_names.h, which bubbles.c includes
 * first, gives twelve of these functions the port's names (util.hpp,
 * ai.hpp, crt.hpp, entity.hpp, cmd_shared.hpp, effects.hpp), so every
 * caller reaches them. Natively nothing calls these names: the port tail
 * bubbles_port.cpp serves the port's names. */
#ifndef LEGORRR_GAME_BUBBLES_H
#define LEGORRR_GAME_BUBBLES_H

#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Image;
struct Point2F;
struct Lego_Object;

void bubble_initialise(void);                                   /* 004070b0 */
void bubble_load_bubbles(struct BlockRec* config);              /* 00407240 */
int bubble_get_bubble_type(const char* name);                   /* 00407300 */
void bubble_toggle_object_uis_always_visible(void);             /* 00407340 */
int bubble_get_object_uis_always_visible(void);                 /* 00407360 */
void bubble_reset_object_bubble_image(struct Lego_Object* liveObj); /* 00407370 */
void bubble_remove_object(struct Lego_Object* liveObj);         /* 004073a0 */
void bubble_show_power_off(struct Lego_Object* liveObj);        /* 00407410 */
void bubble_show_bubble(struct Lego_Object* liveObj);           /* 00407450 */
void bubble_show_call_to_arms(struct Lego_Object* liveObj);     /* 004074b0 */
void bubble_set_call_to_arms_timer(struct Lego_Object* liveObj,
                                   float timer);                /* 00407510 */
void bubble_show_health_bar(struct Lego_Object* liveObj);       /* 00407540 */
void bubble_draw_all_obj_infos(float elapsedGame);              /* 004075a0 */
void bubble_live_object_update_bubble_image(struct Lego_Object* liveObj,
                                            float elapsedGame,
                                            struct Image** outImage,
                                            struct Point2F* screenPt); /* 004078c0 */
int bubble_callback_draw_obj_info(struct Lego_Object* liveObj,
                                  void* pElapsedGame);          /* 00407960 */
void bubble_live_object_get_bubble_image(struct Lego_Object* liveObj,
                                         struct Image** outImage); /* 00407a10 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_BUBBLES_H */
