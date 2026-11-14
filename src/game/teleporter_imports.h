// SPDX-License-Identifier: LGPL-3.0-or-later
/* Teleporter.c (GAME-044) imports: C declarations of the callees and
 * globals the unit uses from other modules (ADR 0005 step 6; finding 621).
 * Included only by teleporter.c and test_c_boundaries.cpp, so the other C
 * units' symbol numbering does not change (finding 568, "Unit-private
 * imports header"). Each declaration names a definition with C linkage, by
 * the name it has in the matching build.
 *
 * LegoObject.c: legoobject_unit.h. Maths.c: gods98/maths.h. */
#ifndef LEGORRR_GAME_TELEPORTER_IMPORTS_H
#define LEGORRR_GAME_TELEPORTER_IMPORTS_H

#include <stdint.h>

#include "gods98/maths.h"
#include "legoobject_unit.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lego.c's globals: legoGlobs.cameraMain (005560e4, gods98/view.cpp),
 * legoGlobs.cameraFP (005560ec, lego_unit.c) and legoGlobs.viewMode
 * (00556ec8, gods98/engine.cpp). */
extern void* s_zoom_step_context;             /* 005560e4 */
extern int32_t s_context_mode;                /* 00556ec8 */

/* Camera.c: Camera_GetTopdownWorldPos (00436b33). The port's entry takes
 * the engine context instead of the map; its adapter is at the end of
 * camera_control_port.cpp. In the matching build the name is
 * camera_control.c's definition (as lego_unit_imports.h). */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define camera_topdown_position_get_adapter camera_get_topdown_world_pos
#endif
int camera_topdown_position_get_adapter(void* camera, void* map,
                                        float out[3]);        /* 00436b33 */

/* 00431570 Lego_GetMap takes no arguments. lego_unit.c defines it with the
 * port's two ignored cell arguments, so the C matching build calls it
 * through an unprototyped function type, which pushes none, as the original
 * does; the native C++ compile goes through lego.cpp's adapter (as
 * camera_control_imports.h). */
void* level_current_terrain_get(int cell_x, int cell_y);      /* 00431570 */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define TELEPORTER_LEGO_GET_MAP() level_current_terrain_get_original()
#else
#define TELEPORTER_LEGO_GET_MAP() ((void* (*)())level_current_terrain_get)()
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TELEPORTER_IMPORTS_H */
