// SPDX-License-Identifier: LGPL-3.0-or-later
/* NERPs.c (GAME-028) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 586). Included only
 * by tutorial_script.c and test_c_boundaries.cpp, so the other C units'
 * symbol numbering does not change (finding 568, "Unit-private imports
 * header"). Each declaration names a definition with C linkage, with the
 * port's parameter types; tutorial_script.c casts its original-layout
 * records to them. */
#ifndef LEGORRR_GAME_TUTORIAL_SCRIPT_IMPORTS_H
#define LEGORRR_GAME_TUTORIAL_SCRIPT_IMPORTS_H

#include <stdint.h>

#include "gods98/files.h"
#include "gods98/images.h"
#include "gods98/sound3d.h"
#include "advisor.h"
#include "camera_control.h"
#include "terrain_map.h"

/* G98-003 Viewports.c (finding 593): Viewport_WorldToScreen keeps the
 * unit's name in the matching build, because C++ port code calls the port's
 * name with the port's projection adapter (viewports_link_names.h). A
 * #define adds no declaration, so the symbol numbering does not move. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define camera_project_point_to_screen viewport_world_to_screen
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct LevelState;
struct ViewProjectionAdapter;

/* GAME-021 Lego.c (lego_unit.c, lego.cpp). */
extern uint32_t legoGlobs_flags1;                  /* 00556ecc flags1 */
extern struct Image* legoGlobs_TutorialIcon;       /* 0055645c TutorialIcon */
struct LevelState* level_current_owner_get(void);  /* 00431560 Lego_GetLevel */
float global_float_field_00556f00_get(void);       /* 004230b0 Lego_GetGameSpeed */
void global_scroll_guard_set(int value);           /* 00425cd0 */
void lego_game_speed_set(float speed);             /* 00425ce0 Lego_SetGameSpeed */
void engine_global_state_flags_bits6_and_9_set_target(int value,
                                                      int set_bit9); /* 00426360 */
void engine_global_state_flags_bit7_set_target(int value);           /* 004263a0 */
void media_action_feedback_sound_dispatch(int event);                /* 00428800 */
void lego_viewport_target_position_clamp(struct EntitySlot* liveObj,
                                         const int* blockPos,
                                         int smooth);       /* 0042de40 */
int lego_object_pointer_registry_get_slot(unsigned int index,
                                          void** objOut);   /* 0042dff0 */
int lego_entity_type_pair_lookup(const char* objName, int* outType,
                                 unsigned int* outID,
                                 void** outModel);          /* 0042e8b0 */

/* GAME-023 LegoObject.c (legoobject_unit.c, legoobject_port.cpp, lego.cpp).
 * LegoObject_GetBlockPos (004424f0): the port's entry takes the level as
 * well, so natively the unit calls lego.cpp's original-ABI adapter. In the
 * C matching build both entries are legoobject_unit.c's (#610, as
 * legoobject_port_aliases.h maps them). */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
#define entity_transform_position_get_outputs legoobject_transform_position_get /* 00442260 */
#define entity_position_to_terrain_cell_adapter legoobject_position_to_terrain_cell /* 004424f0 */
#endif
unsigned int legoobject_grid_record_count_get(int row, int column,
                                              int slot);    /* 004374a0 */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y);   /* 00442260 */
void entity_position_to_terrain_cell_adapter(const struct EntitySlot* entity,
                                             int* cell_x, int* cell_y);

/* GAME-029 Objective.c (objective_unit.cpp, objective.hpp). */
void objective_announce_current(int who);                   /* 00458670 */

/* GAME-041 SFX.c (sfx.cpp, sound.hpp). */
int sound_subsystem_enabled(void);                          /* 00465370 */
void sound_deferred_play_expire(void);                      /* 00464e80 */
int sound_deferred_timer_set_if_idle(float seconds);        /* 00464ec0 */

/* GAME-045 Text.c (text.cpp, message.hpp, frontend.hpp). The original's
 * Text_SetMessage (0046ac20) takes the message line; the port's request
 * word is that pointer on Win32. */
extern uint32_t s_overlay_word_03610;                       /* 00503610 */
extern uint32_t s_overlay_word_03614;                       /* 00503614 */
void ui_message_overlay_state_initialize(void);             /* 0046aa90 */
void advisor_text_request_setup(uint32_t request, uint32_t flags); /* 0046ac20 */

/* G98 Viewports.c (viewports.cpp, view.hpp): Viewport_WorldToScreen. */
void camera_project_point_to_screen(
    const struct ViewProjectionAdapter* adapter, float* output_xy,
    const float* input_xyz);                                /* 004772d0 */

/* G98 Flic.c (flic_unit.cpp): the NERPs function table's "True" row. */
uint32_t flic_opcode25_success_stub(void);                  /* 00484b90 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TUTORIAL_SCRIPT_IMPORTS_H */
