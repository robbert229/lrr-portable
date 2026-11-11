// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-033 Pointer.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-033.csv. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
#include "objective.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/engine.hpp"
#include "gods98/simtick.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "gods98/input.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "gods98/view.hpp"
#include "gods98/string.hpp"
#include "gods98/resource.hpp"
#include "frontend_for_pointer.hpp"
#include <float.h>
#include <errno.h>
#include "crt/crt_for_pointer.hpp"
#include "priorities.h"
#include <stddef.h>
#include "platform/platform.h"
#include "frameloop.hpp"
#include "ui_list_for_pointer.hpp"
#include "gods98/media_for_pointer.hpp"

/* The corpus's override gate compares against this .rdata zero dword
 * (objdump-verified fcomp operand of PointerCursorSelect); a named const
 * static keeps the fcomp operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e64c
static const float s_cursor_gate = 0.0f;   /* DAT_0049e64c, verified */

// GLOBAL: LEGORR 0x004a8a14
static const char s_pointer_name_surface_recharge_seam[] = "Pointer_SurfaceType_RechargeSeam";
// GLOBAL: LEGORR 0x004a8a38
static const char s_pointer_name_surface_crystal_seam[] = "Pointer_SurfaceType_CrystalSeam";
// GLOBAL: LEGORR 0x004a8a58
static const char s_pointer_name_surface_lake[] = "Pointer_SurfaceType_Lake";
// GLOBAL: LEGORR 0x004a8a74
static const char s_pointer_name_surface_ore_seam[] = "Pointer_SurfaceType_OreSeam";
// GLOBAL: LEGORR 0x004a8a90
static const char s_pointer_name_surface_water[] = "Pointer_SurfaceType_Water";
// GLOBAL: LEGORR 0x004a8aac
static const char s_pointer_name_surface_lava[] = "Pointer_SurfaceType_Lava";
// GLOBAL: LEGORR 0x004a8ac8
static const char s_pointer_name_surface_soil[] = "Pointer_SurfaceType_Soil";
// GLOBAL: LEGORR 0x004a8ae4
static const char s_pointer_name_surface_loose[] = "Pointer_SurfaceType_Loose";
// GLOBAL: LEGORR 0x004a8b00
static const char s_pointer_name_surface_medium[] = "Pointer_SurfaceType_Medium";
// GLOBAL: LEGORR 0x004a8b1c
static const char s_pointer_name_surface_hard[] = "Pointer_SurfaceType_Hard";
// GLOBAL: LEGORR 0x004a8b38
static const char s_pointer_name_surface_immovable[] = "Pointer_SurfaceType_Immovable";
// GLOBAL: LEGORR 0x004a8b58
static const char s_pointer_name_vehicle_clear[] = "Pointer_VehicleClear";
// GLOBAL: LEGORR 0x004a8b70
static const char s_pointer_name_lego_man_clear[] = "Pointer_LegoManClear";
// GLOBAL: LEGORR 0x004a8b88
static const char s_pointer_name_vehicle_go[] = "Pointer_VehicleGo";
// GLOBAL: LEGORR 0x004a8b9c
static const char s_pointer_name_lego_man_go[] = "Pointer_LegoManGo";
// GLOBAL: LEGORR 0x004a8bb0
static const char s_pointer_name_vehicle_cant_go[] = "Pointer_VehicleCantGo";
// GLOBAL: LEGORR 0x004a8bc8
static const char s_pointer_name_lego_man_cant_go[] = "Pointer_LegoManCantGo";
// GLOBAL: LEGORR 0x004a8be0
static const char s_pointer_name_vehicle_pick_up[] = "Pointer_VehiclePickUp";
// GLOBAL: LEGORR 0x004a8bf8
static const char s_pointer_name_lego_man_pick_up[] = "Pointer_LegoManPickUp";
// GLOBAL: LEGORR 0x004a8c10
static const char s_pointer_name_vehicle_cant_pick_up[] = "Pointer_VehicleCantPickUp";
// GLOBAL: LEGORR 0x004a8c2c
static const char s_pointer_name_lego_man_cant_pick_up[] = "Pointer_LegoManCantPickUp";
// GLOBAL: LEGORR 0x004a8c48
static const char s_pointer_name_vehicle_dig[] = "Pointer_VehicleDig";
// GLOBAL: LEGORR 0x004a8c5c
static const char s_pointer_name_lego_man_dig[] = "Pointer_LegoManDig";
// GLOBAL: LEGORR 0x004a8c70
static const char s_pointer_name_vehicle_cant_dig[] = "Pointer_VehicleCantDig";
// GLOBAL: LEGORR 0x004a8c88
static const char s_pointer_name_lego_man_cant_dig[] = "Pointer_LegoManCantDig";
// GLOBAL: LEGORR 0x004a8ca0
static const char s_pointer_name_pick_up_ore[] = "Pointer_PickUpOre";
// GLOBAL: LEGORR 0x004a8cb4
static const char s_pointer_name_cant_pick_up[] = "Pointer_CantPickUp";
// GLOBAL: LEGORR 0x004a8cc8
static const char s_pointer_name_pick_up[] = "Pointer_PickUp";
// GLOBAL: LEGORR 0x004a8cd8
static const char s_pointer_name_cant_dynamite[] = "Pointer_CantDynamite";
// GLOBAL: LEGORR 0x004a8cf0
static const char s_pointer_name_dynamite[] = "Pointer_Dynamite";
// GLOBAL: LEGORR 0x004a8d04
static const char s_pointer_name_cannot_build[] = "Pointer_CannotBuild";
// GLOBAL: LEGORR 0x004a8d18
static const char s_pointer_name_can_build[] = "Pointer_CanBuild";
// GLOBAL: LEGORR 0x004a8d2c
static const char s_pointer_name_not_okay[] = "Pointer_NotOkay";
// GLOBAL: LEGORR 0x004a8d3c
static const char s_pointer_name_okay[] = "Pointer_Okay";
// GLOBAL: LEGORR 0x004a8d4c
static const char s_pointer_name_tutorial_block_info[] = "Pointer_TutorialBlockInfo";
// GLOBAL: LEGORR 0x004a8d68
static const char s_pointer_name_get_out[] = "Pointer_GetOut";
// GLOBAL: LEGORR 0x004a8d78
static const char s_pointer_name_get_in[] = "Pointer_GetIn";
// GLOBAL: LEGORR 0x004a8d88
static const char s_pointer_name_put_down[] = "Pointer_PutDown";
// GLOBAL: LEGORR 0x004a8d98
static const char s_pointer_name_cant_help[] = "Pointer_CantHelp";
// GLOBAL: LEGORR 0x004a8dac
static const char s_pointer_name_help[] = "Pointer_Help";
// GLOBAL: LEGORR 0x004a8dbc
static const char s_pointer_name_zoom[] = "Pointer_Zoom";
// GLOBAL: LEGORR 0x004a8dcc
static const char s_pointer_name_cant_zoom[] = "Pointer_CantZoom";
// GLOBAL: LEGORR 0x004a8de0
static const char s_pointer_name_track_object[] = "Pointer_TrackObject";
// GLOBAL: LEGORR 0x004a8df4
static const char s_pointer_name_radar_pan[] = "Pointer_RadarPan";
// GLOBAL: LEGORR 0x004a8e08
static const char s_pointer_name_cant_reinforce[] = "Pointer_CantReinforce";
// GLOBAL: LEGORR 0x004a8e20
static const char s_pointer_name_reinforce[] = "Pointer_Reinforce";
// GLOBAL: LEGORR 0x004a8e34
static const char s_pointer_name_cant_teleport[] = "Pointer_CantTeleport";
// GLOBAL: LEGORR 0x004a8e4c
static const char s_pointer_name_teleport[] = "Pointer_Teleport";
// GLOBAL: LEGORR 0x004a8e60
static const char s_pointer_name_cant_go[] = "Pointer_CantGo";
// GLOBAL: LEGORR 0x004a8e70
static const char s_pointer_name_go[] = "Pointer_Go";
// GLOBAL: LEGORR 0x004a8e7c
static const char s_pointer_name_clear[] = "Pointer_Clear";
// GLOBAL: LEGORR 0x004a8e8c
static const char s_pointer_name_cant_drill[] = "Pointer_CantDrill";
// GLOBAL: LEGORR 0x004a8ea0
static const char s_pointer_name_drill[] = "Pointer_Drill";
// GLOBAL: LEGORR 0x004a8eb0
static const char s_pointer_name_selected[] = "Pointer_Selected";
// GLOBAL: LEGORR 0x004a8ec4
static const char s_pointer_name_blank[] = "Pointer_Blank";
// GLOBAL: LEGORR 0x004a8ed4
static const char s_pointer_name_standard[] = "Pointer_Standard";

// GLOBAL: LEGORR 0x00500a98
int          s_cursor_slot;       /* DAT_00500a98 role */

// GLOBAL: LEGORR 0x00500a9c
uint32_t s_font_words[16];   /* DAT_00500a9c window */

// GLOBAL: LEGORR 0x00500e1c
const char* s_pointer_cursor_names[56];

// GLOBAL: LEGORR 0x00500efc
float s_cursor_override;   /* _DAT_00500efc role (statics: 0.0) */

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void pointer_cursor_slot_release(PointerCursorSlot* slot);
static int pointer_cursor_value_split(char* value, char** fields, unsigned int capacity);

// FUNCTION: LEGORR 0x0045c830
void ui_pointer_cursor_name_registry_init(void)
{
    s_pointer_cursor_names[0] = s_pointer_name_standard;
    s_pointer_cursor_names[1] = s_pointer_name_blank;
    s_pointer_cursor_names[2] = s_pointer_name_selected;
    s_pointer_cursor_names[3] = s_pointer_name_drill;
    s_pointer_cursor_names[4] = s_pointer_name_cant_drill;
    s_pointer_cursor_names[5] = s_pointer_name_clear;
    s_pointer_cursor_names[6] = s_pointer_name_go;
    s_pointer_cursor_names[7] = s_pointer_name_cant_go;
    s_pointer_cursor_names[8] = s_pointer_name_teleport;
    s_pointer_cursor_names[9] = s_pointer_name_cant_teleport;
    s_pointer_cursor_names[10] = s_pointer_name_reinforce;
    s_pointer_cursor_names[11] = s_pointer_name_cant_reinforce;
    s_pointer_cursor_names[12] = s_pointer_name_radar_pan;
    s_pointer_cursor_names[13] = s_pointer_name_track_object;
    s_pointer_cursor_names[54] = s_pointer_name_cant_zoom;
    s_pointer_cursor_names[55] = s_pointer_name_zoom;
    s_pointer_cursor_names[14] = s_pointer_name_help;
    s_pointer_cursor_names[15] = s_pointer_name_cant_help;
    s_pointer_cursor_names[16] = s_pointer_name_put_down;
    s_pointer_cursor_names[17] = s_pointer_name_get_in;
    s_pointer_cursor_names[18] = s_pointer_name_get_out;
    s_pointer_cursor_names[19] = s_pointer_name_tutorial_block_info;
    s_pointer_cursor_names[20] = s_pointer_name_okay;
    s_pointer_cursor_names[21] = s_pointer_name_not_okay;
    s_pointer_cursor_names[22] = s_pointer_name_can_build;
    s_pointer_cursor_names[23] = s_pointer_name_cannot_build;
    s_pointer_cursor_names[24] = s_pointer_name_dynamite;
    s_pointer_cursor_names[25] = s_pointer_name_cant_dynamite;
    s_pointer_cursor_names[26] = s_pointer_name_pick_up;
    s_pointer_cursor_names[27] = s_pointer_name_cant_pick_up;
    s_pointer_cursor_names[28] = s_pointer_name_pick_up_ore;
    s_pointer_cursor_names[29] = s_pointer_name_lego_man_cant_dig;
    s_pointer_cursor_names[30] = s_pointer_name_vehicle_cant_dig;
    s_pointer_cursor_names[31] = s_pointer_name_lego_man_dig;
    s_pointer_cursor_names[32] = s_pointer_name_vehicle_dig;
    s_pointer_cursor_names[33] = s_pointer_name_lego_man_cant_pick_up;
    s_pointer_cursor_names[34] = s_pointer_name_vehicle_cant_pick_up;
    s_pointer_cursor_names[35] = s_pointer_name_lego_man_pick_up;
    s_pointer_cursor_names[36] = s_pointer_name_vehicle_pick_up;
    s_pointer_cursor_names[37] = s_pointer_name_lego_man_cant_go;
    s_pointer_cursor_names[38] = s_pointer_name_vehicle_cant_go;
    s_pointer_cursor_names[39] = s_pointer_name_lego_man_go;
    s_pointer_cursor_names[40] = s_pointer_name_vehicle_go;
    s_pointer_cursor_names[41] = s_pointer_name_lego_man_clear;
    s_pointer_cursor_names[42] = s_pointer_name_vehicle_clear;
    s_pointer_cursor_names[43] = s_pointer_name_surface_immovable;
    s_pointer_cursor_names[44] = s_pointer_name_surface_hard;
    s_pointer_cursor_names[45] = s_pointer_name_surface_medium;
    s_pointer_cursor_names[46] = s_pointer_name_surface_loose;
    s_pointer_cursor_names[47] = s_pointer_name_surface_soil;
    s_pointer_cursor_names[48] = s_pointer_name_surface_lava;
    s_pointer_cursor_names[49] = s_pointer_name_surface_water;
    s_pointer_cursor_names[50] = s_pointer_name_surface_ore_seam;
    s_pointer_cursor_names[51] = s_pointer_name_surface_lake;
    s_pointer_cursor_names[52] = s_pointer_name_surface_crystal_seam;
    s_pointer_cursor_names[53] = s_pointer_name_surface_recharge_seam;
}

// FUNCTION: LEGORR 0x0045ca70
void pointer_cursor_config_load_target(BlockRec* first_entry)
{
    BlockRec* entry;
    uint32_t* feature_flags;
    if (s_pointer_cursor_runtime.names[0] == NULL)
        pointer_cursor_registry_init(&s_pointer_cursor_runtime);
    feature_flags = engine_current_feature_word();
    for (entry = first_entry; entry != NULL; entry = block_next_sibling(entry)) {
        const char* entry_name = block_name(entry);
        const char* entry_value = block_value(entry);
        char value_copy[512];
        char* fields[3];
        int field_count;
        int slot_index;
        int animated;
        PointerCursorSlot* slot;
        if (entry_name == NULL || entry_value == NULL) continue;
        if (entry_name[0] == '!') {
            if (feature_flags != NULL &&
                (*feature_flags & 0x02000000u) != 0u) continue;
            ++entry_name;
        }
        if (!pointer_cursor_config_parse_target(entry_name, &slot_index) ||
            slot_index < 0 || slot_index >= POINTER_CURSOR_COUNT)
            continue;
        strncpy(value_copy, entry_value, sizeof(value_copy) - 1u);
        value_copy[sizeof(value_copy) - 1u] = '\0';
        field_count = pointer_cursor_value_split(value_copy, fields, 3u);
        if (field_count < 1) continue;
        slot = &s_pointer_cursor_runtime.slots[slot_index];
        animated = field_count != 1;
        if (animated) {
            Flic* animation = flic_open(fields[0]);
            if (animation == NULL) continue;
            pointer_cursor_slot_release(slot);
            slot->resource = animation;
            slot->animated = 1;
            slot->hot_x = field_count > 1 ? atoi(fields[1]) : 0;
            slot->hot_y = field_count > 2 ? atoi(fields[2]) : 0;
        }
        else {
            TextureSurface* bitmap = image_surface_load(fields[0], 0u, 0u);
            if (bitmap == NULL) continue;
            image_surface_blit_transformed(bitmap, 0.0f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 0.0f);
            pointer_cursor_slot_release(slot);
            slot->resource = bitmap;
            slot->animated = 0;
            slot->hot_x = 0;
            slot->hot_y = 0;
        }
    }
}
// FUNCTION: LEGORR 0x0045cbd0
int pointer_cursor_config_parse_target(const char* name, int* index_out)
{
    if (s_pointer_cursor_runtime.names[0] == NULL)
        pointer_cursor_registry_init(&s_pointer_cursor_runtime);
    return pointer_cursor_name_resolve(&s_pointer_cursor_runtime, name,
                                       index_out);
}

// FUNCTION: LEGORR 0x0045cc10
uint32_t indexed_global_value_at_get(int index)
{
    if (index < 0 || index >= 16) {
        return 0u; /* deviation: the corpus strides blindly */
    }
    return s_font_words[index];
}

// FUNCTION: LEGORR 0x0045cc20
void media_pointer_cursor_select(int slot)
{
    if (!(s_cursor_override > s_cursor_gate)) {
        s_cursor_slot = slot;
    }
}

// FUNCTION: LEGORR 0x0045cc40
void media_pointer_cursor_override_set(int slot, float value)
{
    s_cursor_slot = slot;
    s_cursor_override = value;
}

// FUNCTION: LEGORR 0x0045cc60
uint32_t global_cursor_word_get(void)
{
    return s_cursor_slot;
}

// FUNCTION: LEGORR 0x0045cc70
void pointer_cursor_draw_target(uint32_t x, uint32_t y)
{
    DisplaySurface* target = surface_active_render_target_get();
    void* pixels = NULL;
    int pitch = 0;
    if (target == NULL || surface_lock(target, &pixels, &pitch) < 0) return;
    pointer_cursor_draw_pixels(x, y, pixels, pitch, target->w, target->h);
    surface_unlock(target, pixels);
}

// FUNCTION: LEGORR 0x0045cd90
void media_pointer_cursor_override_tick(float delta)
{
    if (s_cursor_override > 0.0f)
        s_cursor_override -= delta;
}

static void pointer_cursor_slot_release(PointerCursorSlot* slot)
{
    if (slot == NULL || slot->resource == NULL) return;
    if (slot->animated)
        (void)flic_close((Flic*)slot->resource);
    else
        texture_release((TextureSurface*)slot->resource);
    slot->resource = NULL;
    slot->animated = 0;
    slot->hot_x = 0;
    slot->hot_y = 0;
}

static int pointer_cursor_value_split(char* value, char** fields,
                                      unsigned int capacity)
{
    unsigned int count = 0u;
    char* p;
    if (value == NULL || fields == NULL || capacity == 0u || value[0] == '\0')
        return 0;
    fields[count++] = value;
    for (p = value; *p != '\0' && count < capacity; ++p) {
        if (*p == ',') {
            *p = '\0';
            fields[count++] = p + 1;
        }
    }
    return (int)count;
}
