// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): LegoObject.c (GAME-023) entries declared in entity.hpp, and the
 * port-tail adapter for LegoObject_GetBlockPos.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_LEGOOBJECT_H
#define LEGORRR_GAME_LEGOOBJECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;
struct GameContext;

int entity_teardown(struct Lego_Object* entity);             /* 00437540 */
struct Lego_Object* entity_create_with_position(struct GameContext* context,
                                               int type, int id,
                                               int last, float x, float y,
                                               float heading); /* 00438610 */
void entity_action_route_state_initialize(struct Lego_Object* entity); /* 00442210 */
void entity_transform_position_get_outputs(const struct Lego_Object* entity,
                                           float* out_x,
                                           float* out_y); /* 00442260 */
void entity_surface_vector_normalize(void* surface, float out_xy[2]); /* 004422a0 */
/* 004424f0 LegoObject_GetBlockPos. The port entry also takes the level;
 * this adapter, in the GAME-013 port tail, passes the current one. */
int legoobject_block_position_get(struct Lego_Object* entity, int* bx,
                                   int* by);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_H */
