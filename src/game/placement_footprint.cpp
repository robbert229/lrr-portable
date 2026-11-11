// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-040 GAME-040: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-040.csv. */
#include "record.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "view.hpp"
#include "texture.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "game/util.hpp"
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "record_shared.hpp"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "model.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include "view_shared.hpp"

// GLOBAL: LEGORR 0x005013c8
int s_rot_pairs[ROT_PAIR_SLOTS * 2];

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void surface_visibility_set_target( const void* record, uint32_t flag);

// FUNCTION: LEGORR 0x00464110
void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation)
{
    if (count > ROT_PAIR_SLOTS) {
        count = ROT_PAIR_SLOTS; /* deviation: the corpus fills blindly */
    }
    for (int n = 0; n < count; ++n) {
        const int t = rotation + 0x8000; /* the corpus's sign fold */
        const int u = t >> 31;
        const int sel = (((t ^ u) - u & 3) ^ u) - u;
        int x = source[2 * n];
        int y = source[2 * n + 1];
        switch (sel) {
        case 1: { const int h = x; x = -y; y = -h; } break;
        case 2:  x = -x; y = -y; break;
        case 3: { const int h = x; x = y; y = -h; } break;
        default: break; /* identity (negative folds land here) */
        }
        s_rot_pairs[2 * n]     = x + offset_pair[0];
        s_rot_pairs[2 * n + 1] = y + offset_pair[1];
    }
    return s_rot_pairs;
}

/* SurfaceVisibilitySet: owned software meshes use their real visibility;
 * other records retain the target's record +0 handle/scene operation. */
// FUNCTION: LEGORR 0x00464720
void surface_visibility_set(const void* record, uint32_t flag)
{
    SurfaceVisibilityAdapter adapter = s_surface_visibility_adapter;
    void* context = s_surface_visibility_context;
    if (adapter != NULL && adapter(context, record, flag) != 0) return;
    surface_visibility_set_target(record, flag);
}
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) void surface_visibility_set_target(
    const void* record, uint32_t flag)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        push eax
        mov edx, dword ptr [ecx]
        push edx
        call scene_render_lock_toggle
        add esp, 8
        ret
    }
}
#else
static void surface_visibility_set_target(const void* record, uint32_t flag)
{
    scene_render_lock_toggle(rec_ptr(record, 0x00), (int)flag);
}
#endif
