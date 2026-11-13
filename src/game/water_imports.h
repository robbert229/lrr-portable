// SPDX-License-Identifier: LGPL-3.0-or-later
/* Water.c (GAME-049) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 614). Included only by
 * water.c and test_c_boundaries.cpp, so the other C units' symbol numbering
 * does not change (finding 568, "Unit-private imports header"). Each
 * declaration names a definition with C linkage, by the name it has in the
 * matching build.
 *
 * Container.c: the mesh functions are gods98/mesh.h's and Container_Hide
 * (00473b40) is gods98/record.h's. Container_Mesh_AddGroup (004748f0)
 * keeps the port's contract, a table of 16-bit indices; the original passes
 * 32-bit face data, so the unit casts it (the cast changes no code).
 * Map3D.c: terrain_map.h. Lego.c: lego_unit.h. */
#ifndef LEGORRR_GAME_WATER_IMPORTS_H
#define LEGORRR_GAME_WATER_IMPORTS_H

#include "gods98/mesh.h"
#include "gods98/record.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Container_SetColourAlpha (FUN_004750b0). */
void render_object_set_packed_rgba_from_float_channels(
    void* scene_object, float red, float green, float blue, float alpha);

#ifdef __cplusplus
}
#endif

/* Water_ChangeViewMode's callee is an empty function that the linker
 * folded to 00484c90 (flic_opcode27_fallback_noop, flic.hpp); the original
 * passes the pool's mesh and three colour channels. */
#if defined(_MSC_VER) && _MSC_VER < 1200
void flic_opcode27_fallback_noop(void* mesh, float red, float green,
                                 float blue);
#define water_set_emissive flic_opcode27_fallback_noop
#else
/* PORT-ONLY: flic.hpp declares the empty function without parameters in
 * C++, and the call does nothing, so other compilers drop it. */
#define water_set_emissive(mesh, red, green, blue) ((void)(mesh))
#endif

#endif /* LEGORRR_GAME_WATER_IMPORTS_H */
