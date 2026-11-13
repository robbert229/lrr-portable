// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-014 Encyclopedia.c's port tail (finding 633): the port's models of
 * the unit's functions and the state the port's other files read.
 * Unannotated: the original-shape unit is encyclopedia.c (ADR 0005). In the
 * matching build the unit defines the four port names its callers use
 * (encyclopedia_link_names.h), so this file compiles its own out there.
 * Relocated by tools/relocate_unit.py (ADR 0006); body hashes:
 * docs/coverage/relocations/GAME-014.csv. */
/* Matching build: LegoObject.c callers call legoobject_unit.c (#575). */
#include "legoobject_port_aliases.h"
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gods98/texture.hpp"
#include "crt/crt.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "crt/crt_for_encyclopedia.hpp"
#include <stdint.h>
#include "ai.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/record.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "entity_for_encyclopedia.hpp"
#include <stdarg.h>
#include <time.h>
#include "frontend.h"
#include "objective.hpp"
#include "terrain_feedback.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/simtick.hpp"
#include "gods98/flic.hpp"
#include "gods98/surface.hpp"
#include "gods98/input.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "frontend_for_encyclopedia.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/camera.hpp"
#include "scene_render.hpp"
#include "level_for_encyclopedia.hpp"
#include "crt/crt_for_encyclopedia.hpp"
#include "entity_for_encyclopedia.hpp"
#include "frontend_for_encyclopedia.hpp"
#include "level_for_encyclopedia.hpp"

/* ItemNameByKindLookup's fixed labels: Encyclopedia.c's literals "Ore"
 * (004a1e98) and "PowerCrystal" (004a1e9c), which encyclopedia.c marks. */
const char s_item_name_ore[] = "Ore";
const char s_item_name_power_crystal[] = "PowerCrystal";

/* The port's models of encyclopediaGlobs.currObject (004c7ea4) and
 * .flags (004c7ea8); encyclopedia.c owns the original's. */
int s_global_value_word;
uint32_t s_ui_flag_word = 0u;

#if !(defined(_MSC_VER) && _MSC_VER < 1200)
/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void encyclopedia_value_open(BlockFile* cfg, const char* root_path, const char* name, void** slot);
#endif

/* The port's model of 0x0040e490; encyclopedia.c serves this name in the
 * matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void encyclopedia_config_files_open(BlockFile* cfg, const char* root_path)
{
    static const int count_slot[4] = { 0, 2, 3, 1 }; /* 0x27c/80/84/88 */
    for (int kind = 0; kind != 4; ++kind) {
        const uint32_t count = catalog_count_peek(count_slot[kind]);
        /* The corpus strides count 4-byte handle slots (its windows are
         * undefined4 arrays sized count * 4); on the pointer-sized port the
         * window is count void* slots, so the allocation and zeroing stride
         * sizeof(void*) to cover every handle (deviation: slot width). */
        void* array = engine_heap_alloc((long)count * (long)sizeof(void*));
        s_ency_files[kind] = array;
        if (array == NULL) {
            continue; /* deviation: the corpus strides the store blindly */
        }
        memset(array, 0, (size_t)count * sizeof(void*));
        const char* const* table =
            (const char* const*)catalog_name_table_peek(kind);
        for (uint32_t i = 0; i != count; ++i) {
            const char* name = (table != NULL) ? table[i] : NULL;
            encyclopedia_value_open(cfg, root_path, name,
                                    &((void**)array)[i]);
        }
    }
    encyclopedia_value_open(cfg, root_path, "PowerCrystal", &s_ency_crystal);
    encyclopedia_value_open(cfg, root_path, "Ore", &s_ency_ore);
}
#endif

/* The port's model of 0x0040e700; encyclopedia.c serves this name in the
 * matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void terrain_choice_resource_select(const void* record)
{
    uint32_t kind = 0u, index = 0u;
    copy_two_dword_pair_to_outputs(record, &kind, &index);
    switch (kind) {
    case 1u: case 2u: case 3u: case 4u: {
        const TerrainChoiceWindow* window = &s_choice_windows[kind - 1u];
        s_choice_active = index < window->count
            ? window->values[index] : 0u;
        break;
    }
    case 6u: s_choice_active = s_choice_scalar[0]; break;
    case 7u: s_choice_active = s_choice_scalar[1]; break;
    default:
        s_choice_active = 0u;
        s_choice_flags &= ~1u;
        return;
    }
    s_choice_flags |= 3u;
    s_choice_record = (uintptr_t)record;
    ui_text_output_object_release((void* const*)panel_text_slot_peek());
}
#endif

/* The port's model of 0x0040e7e0; encyclopedia.c serves this name in the
 * matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void global_flag_bit0_clear(void)
{
    /* corpus: mov eax,ds:004c7ea8 / and al,0FEh / mov ds:004c7ea8,eax.
     * The plain load-op-store spelling (not the &= compound) keeps the
     * /Od register round-trip instead of folding to and mem,imm; the
     * corpus's byte-sized and is MSVC's own narrowing of the mask. */
    s_ui_flag_word = s_ui_flag_word & ~1u;
}
#endif

/* The port's model of 0x0040e910; encyclopedia.c serves this name in the
 * matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void global_value_clear_if_equal(int value)
{
    if (s_global_value_word == value) {
        s_global_value_word = 0;
    }
}
#endif

/* The corpus joins `<root>::Encyclopedia::<name>` through the path builder
 * (FUN_00478f50), reads the record's +8 value token (FUN_004790b0's
 * record+8), and dispatches the file open (FUN_0047f6e0 in "r" mode — the
 * port's AUTO open). NULL answers ride where the corpus strides. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
static void encyclopedia_value_open(BlockFile* cfg, const char* root_path,
                                    const char* name, void** slot)
{
    const char* path = string_path_join(root_path, "Encyclopedia", name,
                                        (char*)NULL);
    const char* value = block_get_strptr(cfg, path);
    if (value != NULL) {
        *slot = resource_open(value, RESOURCE_AUTO);
    }
}
#endif
