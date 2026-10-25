// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — corpus record access primitives (see record.hpp). */

#include "record.hpp"

/* Byte-safe field reads: corpus dword/float slots at offsets that may
 * not be pointer-aligned on 64-bit hosts. */
static uint32_t rec_dword(const void* base, size_t off)
{
    uint32_t v = 0;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

static float rec_float(const void* base, size_t off)
{
    float v = 0.0f;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

/* Pointer slots are modeled native-size (documented deviation from the
 * corpus's 32-bit pointers); reads stay byte-safe like the fields. */
static void* rec_ptr(const void* base, size_t off)
{
    void* v = NULL;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

/* Contents of a native cell array: memcpy read (the corpus's cell
 * dwords can sit unaligned after a stride change). */
static uint32_t cells_dword(const void* cells, size_t off)
{
    uint32_t v = 0;
    if (cells != NULL) {
        __builtin_memcpy(&v, (const char*)cells + off, sizeof(v));
    }
    return v;
}

/* RecordField14Get (FUN_00435f0b). */
uint32_t record_field14_get(const void* record)
{
    return rec_dword(record, 0x14);
}

/* ObjectField10Get (FUN_004850e0). */
uint32_t object_field10_get(const void* object)
{
    return rec_dword(object, 0x10);
}

/* RecordFieldD8EqualsOne (FUN_0046c3d0): int == 1, not "nonzero". */
int record_field_d8_equals_one(const void* record)
{
    const int32_t v = (int32_t)rec_dword(record, 0xd8);
    return v == 1;
}

/* ReadIndexedPairSlotDword (FUN_00471a20): base + i*8 + j*4, exactly as
 * the corpus computes it. */
uint32_t read_indexed_pair_slot_dword(const void* base, uint32_t i,
                                      uint32_t j)
{
    return rec_dword(base, (size_t)i * 8u + (size_t)j * 4u);
}

/* VelocitySlotPrimaryObjectGet / SecondaryObjectGet (FUN_00471a40 /
 * FUN_00471a50): indexed slot at +0x18 / +0x24. */
uint32_t velocity_slot_primary_object_get(const void* record,
                                          uint32_t index)
{
    return rec_dword(record, 0x18u + (size_t)index * 4u);
}

uint32_t velocity_slot_secondary_object_get(const void* record,
                                            uint32_t index)
{
    return rec_dword(record, 0x24u + (size_t)index * 4u);
}

/* CategoryValueIsNeitherZeroNorFifteen (FUN_0041b470). */
int category_value_is_neither_zero_nor_fifteen(int value)
{
    if (value != 0 && value != 0xf) {
        return 1;
    }
    return 0;
}

/* InterpolatedZWithinBounds (FUN_00471330): corpus order — the
 * interpolation, then `v <= target` and `target - t <= v`. */
int interpolated_z_within_bounds(const void* record_a, const void* record_b,
                                 float t, float target, float scale)
{
    const float v = rec_float(record_b, 8) * scale + rec_float(record_a, 8);
    if (v <= target && target - t <= v) {
        return 1;
    }
    return 0;
}

/* CameraRotationGet (FUN_00436b79): the float at +0x84 (opaque-record
 * reader; see header). */
float camera_rotation_get(const void* camera)
{
    return rec_float(camera, 0x84);
}

/* MapCellToWorldCenter (FUN_0044f640, finding 225): the descriptor's
 * int fields convert to float at the use site — reproduced exactly,
 * including the unsigned bounds compare. */
int map_cell_to_world_center(const void* map, uint32_t cell_x,
                             uint32_t cell_y, float* world_x, float* world_y)
{
    const uint32_t width = rec_dword(map, 0x00);
    const uint32_t height = rec_dword(map, 0x04);
    const int32_t scale_i = (int32_t)rec_dword(map, 0x10);
    const int32_t origin_x_i = (int32_t)rec_dword(map, 0x18);
    const int32_t origin_y_i = (int32_t)rec_dword(map, 0x1C);
    const float scale = (float)scale_i;
    const float origin_x = (float)origin_x_i;
    const float origin_y = (float)origin_y_i;

    if (cell_x < width && cell_y < height) {
        if (world_x != NULL) {
            *world_x = scale * 0.5f + (float)cell_x * scale + origin_x;
        }
        if (world_y != NULL) {
            *world_y = origin_y - (scale * 0.5f + (float)cell_y * scale);
        }
        return 1;
    }
    return 0;
}

/* TaskFlagBitQuery (FUN_00408970, findings 366/465). */
uint32_t task_flag_bit_query(const void* task, uint32_t bit, int suppress)
{
    const uint32_t mask = 1u << (bit & 0x1fu);
    const uint32_t flags = rec_dword(task, 0x98);
    if (suppress != 0 && (flags & mask) != 0) {
        return 0;
    }
    return flags & mask;
}

/* TaskField84GetAndField88Out (FUN_00408360): the pair read, corpus
 * order — the optional +0x88 write happens before the +0x84 answer. */
uint32_t task_field84_get_and_field88_out(const void* task, uint32_t* out88)
{
    if (task == NULL) {
        return 0u;
    }
    if (out88 != NULL) {
        *out88 = rec_dword(task, 0x88);
    }
    return rec_dword(task, 0x84);
}

/* PackedGridCellValueGet (FUN_0040b850, findings 373/451): low nibble
 * * width + high nibble indexes the cell dwords; the corpus's own NULL
 * grid guard answers 0, reproduced. */
uint32_t packed_grid_cell_value_get(const void* grid, uint32_t packed_cell)
{
    void* cells = rec_ptr(grid, 0x08);
    if (cells == NULL) {
        return 0u;
    }
    const int32_t width = (int32_t)rec_dword(grid, 0x00);
    const int32_t index = (int32_t)(packed_cell & 0xfu) * width
        + (int32_t)((packed_cell & 0xffu) >> 4);
    return cells_dword(cells, (size_t)index * 4u);
}

/* ObjectPoolListHeadGet (FUN_00456c30, finding 374): the corpus's exact
 * in-range bounds (-1 < index < 0x37) over the +0x114 slot table. */
int object_pool_list_head_get(const void* pools, int index)
{
    if (-1 < index && index < 0x37) {
        const int32_t v = (int32_t)rec_dword(
            pools, (size_t)(0x114 + index * 4));
        if (v != 0) {
            return v;
        }
    }
    return 0;
}

/* ModelChildRenderEligibilityCheck (FUN_00483a70): the two-bit render
 * gate (finding 431). NULL answers "not eligible" — the corpus would
 * crash on it (documented deviation). */
int model_child_render_eligibility_check(const void* object)
{
    if (object == NULL) {
        return 0;
    }
    const uint32_t flags = rec_dword(object, 0x6c);
    if ((flags & 1u) == 0 && (flags & 0x10000000u) == 0) {
        return 1;
    }
    return 0;
}

/* EntityTypeFlagBit1Get (FUN_0046d330, findings 357/432). */
uint32_t entity_type_flag_bit1_get(const void* record)
{
    return (rec_dword(record, 0x1e8) & 0xffu) >> 1 & 1u;
}

/* ------------------------------------------------------------------ *
 *  Field-reader sweep (see record.hpp). */

uint32_t object_record_field11c_get(const void* record)
{
    return rec_dword(record, 0x11c);
}

uint32_t object_field_be4_dword_get(const void* record)
{
    return rec_dword(record, 0xbe4);
}

uint32_t entity_type_mask_get(const void* record)
{
    return rec_dword(record, 300);
}

uint32_t pointer_value_get(const void* pointed)
{
    return rec_dword(pointed, 0);
}

uint32_t scene_object_flags_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x70);
}

uint32_t scene_object_owner_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x20);
}

uint32_t scene_object_resource_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x08);
}

uint32_t scene_object_wrapped_pointer_get(const void* wrapped)
{
    return rec_dword(wrapped, 0);
}

uint32_t scene_object_flag_bit3_get(const void* scene_object)
{
    return (rec_dword(scene_object, 0x14) & 0xffu) >> 3 & 1u;
}

int scene_object_mode_is_one(const void* scene_object)
{
    return rec_dword(scene_object, 0x28) == 1u ? 1 : 0;
}

uint32_t animation_object_handle_get(const void* animation_object)
{
    return rec_dword(animation_object, 0x04);
}

uint32_t resource_entry_id_get(const void* entry)
{
    if (entry != NULL) {
        uint16_t v = 0;
        __builtin_memcpy(&v, (const char*)entry + 2, sizeof(v));
        return v;
    }
    return 0u;
}

uint32_t indexed_word_table_lookup(uint32_t index, const void* table)
{
    const size_t off = 0x41c + (size_t)(index & 0xffu) * 2u;
    if (table != NULL) {
        uint16_t v = 0;
        __builtin_memcpy(&v, (const char*)table + off, sizeof(v));
        return v;
    }
    return 0u;
}

uint32_t indexed_record_flag6c_bit0_get(const void* record, int index)
{
    const char* base = (const char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return 0u;
    }
    const size_t off = (size_t)index * 0x70u + 0x6cu;
    return rec_dword(base + off, 0) & 1u;
}

void* indexed_record_data_address_get(const void* record, int index)
{
    char* base = (char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return NULL;
    }
    return (void*)(base + (size_t)((size_t)index * 0x70u + 0x10u));
}

int count_matching_coordinates_in_list(const void* manager,
                                       int32_t coord_x, int32_t coord_y)
{
    const int32_t count = (int32_t)rec_dword(manager, 0x20);
    if (count == 0) {
        return 0;
    }
    int found = 0;
    for (int32_t i = 0; i < count; ++i) {
        void* entry = rec_ptr(manager, 0x2cu + (size_t)i * sizeof(void*));
        if (entry != NULL
            && (int32_t)rec_dword(entry, 0) == coord_x
            && (int32_t)rec_dword(entry, 4) == coord_y) {
            ++found;
        }
    }
    return found;
}

/* AviPlayerReadyGet (FUN_0047ed80, findings 185/414): the readiness
 * byte at +0x99 (byte-safe — the corpus reads it as a raw byte). */
uint32_t avi_player_ready_get(const void* movie)
{
    if (movie != NULL) {
        uint8_t v = 0;
        __builtin_memcpy(&v, (const char*)movie + 0x99, sizeof(v));
        return v;
    }
    return 0u;
}

/* GameFileSystemTypeGet (FUN_0047fe20, findings 234/404): the handle's
 * leading type dword. */
uint32_t game_file_system_type_get(const void* handle)
{
    return rec_dword(handle, 0);
}

/* IndexedRecordTransformCopyIn (FUN_00483240, findings 372/431): the
 * inline-block copy into the indexed record's data slot. */
uint32_t indexed_record_transform_copy_in(void* record, int index,
                                          const void* source)
{
    void* dst = indexed_record_data_address_get(record, index);
    if (dst != NULL && source != NULL) {
        __builtin_memcpy(dst, source, 0x50u);
    }
    return 1u;
}

/* --- grid-descriptor coordinate leaves (see record.hpp) ------------------ */

int world_to_terrain_cell_bounds_check(const void* map, float world_x,
                                       float world_y, int* cell_x,
                                       int* cell_y, float* subcell)
{
    const int cx = (int)world_x; /* __ftol: truncate toward zero */
    const int cy = (int)world_y;
    if (cell_x != NULL) {
        *cell_x = cx;
    }
    if (cell_y != NULL) {
        *cell_y = cy;
    }
    const int w = (int)rec_dword(map, 0x00);
    const int h = (int)rec_dword(map, 0x04);
    if (map == NULL || !(-1 < cx && cx < w && -1 < cy && cy < h)) {
        return 0;
    }
    if (subcell != NULL) {
        const float bs = (float)(int)rec_dword(map, 0x10);
        const float ox = (float)(int)rec_dword(map, 0x18);
        const float oy = (float)(int)rec_dword(map, 0x1C);
        float px = (world_x - ((float)cx * bs + ox)) / (bs * 0.5f);
        float py = -((world_y - (oy - (float)cy * bs)) / (bs * 0.5f));
        if (1.0f < px) {
            px = 2.0f - px;
        }
        if (1.0f < py) {
            py = 2.0f - py;
        }
        if (px < py) {
            py = px;
        }
        *subcell = py;
    }
    return 1;
}

void grid_coordinate_clamp_and_store(const void* map, float world_x,
                                     float world_y, int* cell_x, int* cell_y)
{
    const int w = (int)rec_dword(map, 0x00);
    const int h = (int)rec_dword(map, 0x04);
    int cx = (int)world_x;
    int cy = (int)world_y;
    if (cx < 0) {
        cx = 0;
    }
    if (w <= cx) {
        cx = w - 1;
    }
    if (cy < 0) {
        cy = 0;
    }
    if (h <= cy) {
        cy = h - 1;
    }
    if (cell_x != NULL) {
        *cell_x = cx;
    }
    if (cell_y != NULL) {
        *cell_y = cy;
    }
}

uint32_t terrain_world_point_to_cell(const void* map, const float* point,
                                     float* cell_out, float* offset_out)
{
    int cx = 0, cy = 0;
    if (point != NULL
        && world_to_terrain_cell_bounds_check(map, point[0], point[1],
                                              &cx, &cy, NULL) != 0) {
        if (cell_out != NULL) {
            cell_out[0] = (float)cx;
            cell_out[1] = (float)cy;
        }
        if (offset_out != NULL) {
            float wx = 0.0f, wy = 0.0f;
            map_cell_to_world_center(map, (uint32_t)cx, (uint32_t)cy,
                                     &wx, &wy);
            float scale_f = 0.0f;
            __builtin_memcpy(&scale_f, (const char*)map + 0x10,
                             sizeof(scale_f));
            const float inv = 1.0f / scale_f;
            offset_out[0] = inv * (point[0] - wx) + 0.5f;
            offset_out[1] = inv * (point[1] - wy) + 0.5f;
        }
    }
    return 0u;
}

/* --- indexed flag slot and resource-handle writers (see record.hpp) ------ */

void indexed_flag_value_set_one(void* record, uint32_t index)
{
    if (record == NULL) {
        return;
    }
    const uint32_t one = 1u;
    const size_t off = 0x40b4u + (size_t)(index & 0xffu) * 4u;
    __builtin_memcpy((char*)record + off, &one, sizeof(one));
}

void scene_object_resource_handle_set(void* object, uint32_t handle)
{
    if (object == NULL) {
        return;
    }
    __builtin_memcpy((char*)object + 0x2cu, &handle, sizeof(handle));
}

/* --- entity record-field leaves (see record.hpp) --------------------------- */
void record_fields_4_and_14_set(void* record, uint32_t value_a,
                                uint32_t value_b)
{
    if (record == NULL) {
        return;
    }
    __builtin_memcpy((char*)record + 0x04, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 0x14, &value_b, sizeof(value_b));
}

void entity_fields_a0_a4_store_a8_clear(void* record, uint32_t value_a,
                                        uint32_t value_b)
{
    if (record == NULL) {
        return;
    }
    const uint32_t zero = 0u;
    __builtin_memcpy((char*)record + 0xa0, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 0xa4, &value_b, sizeof(value_b));
    __builtin_memcpy((char*)record + 0xa8, &zero, sizeof(zero));
}

void entity_b8_bit3_set_or_clear(void* record, int set)
{
    if (record == NULL) {
        return;
    }
    unsigned char* byte = (unsigned char*)record + 0xb8;
    if (set != 0) {
        *byte = (unsigned char)(*byte | 8);
    } else {
        *byte = (unsigned char)(*byte & ~8u);
    }
}

/* --- the record-field write band (see record.hpp) --------------------------- */
void record_field18_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x18, &value, sizeof(value));
}

void record_flags_24_or(void* record, uint32_t flags)
{
    if (record == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    uint32_t f = 0u;
    __builtin_memcpy(&f, (char*)record + 0x24, sizeof(f));
    f |= flags;
    __builtin_memcpy((char*)record + 0x24, &f, sizeof(f));
}

void object_field14_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x14, &value, sizeof(value));
}

void linked_record_field4_clear(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    char* node = (char*)record;
    while (node != NULL) {
        const uint32_t zero = 0u;
        __builtin_memcpy(node + 4, &zero, sizeof(zero));
        void* next = NULL;
        __builtin_memcpy(&next, node + 0x14, sizeof(next));
        node = (char*)next;
    }
}

/* CopyTwoDwordPairToOutputs (FUN_00439930) — see record.hpp. */
void copy_two_dword_pair_to_outputs(const void* record, uint32_t* out_a,
                                    uint32_t* out_b)
{
    if (record == NULL || out_a == NULL || out_b == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy(out_a, (const char*)record, 4);
    __builtin_memcpy(out_b, (const char*)record + 4, 4);
}

void scene_object_owner_set(void* scene_object, uint32_t owner)
{
    if (scene_object == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)scene_object + 0x20, &owner, sizeof(owner));
}

/* --- the record primitive band (see record.hpp) ----------------------------- */
void indexed_record_field64_set(void* record, int index, uint32_t value)
{
    void* base = NULL;
    if (record == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    __builtin_memcpy(&base, (char*)record + 8, sizeof(base));
    if (base == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy((char*)base + 100 + (size_t)index * 0x70,
                     &value, sizeof(value));
}

void object_vector3_set(void* object_block, float x, float y, float z)
{
    void* base = NULL;
    if (object_block == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    __builtin_memcpy(&base, (char*)object_block, sizeof(base));
    if (base == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy((char*)base + 0x3c, &x, sizeof(x));
    __builtin_memcpy((char*)base + 0x40, &y, sizeof(y));
    __builtin_memcpy((char*)base + 0x44, &z, sizeof(z));
}

void float_pair_at_offset4_set_from_ints(void* record, int x, int y)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const float fx = (float)x; /* the corpus's int->float casts */
    const float fy = (float)y;
    __builtin_memcpy((char*)record + 0x04, &fx, sizeof(fx));
    __builtin_memcpy((char*)record + 0x08, &fy, sizeof(fy));
}

void float_pair_at_offsetc_set_from_uints(void* record, uint32_t x,
                                          uint32_t y)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const float fx = (float)x; /* the corpus's uint->float casts */
    const float fy = (float)y;
    __builtin_memcpy((char*)record + 0x0c, &fx, sizeof(fx));
    __builtin_memcpy((char*)record + 0x10, &fy, sizeof(fy));
}

void object_state_pair_get(const void* record, uint32_t* out_a,
                           uint32_t* out_b)
{
    if (record == NULL || out_a == NULL || out_b == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy(out_a, (const char*)record + 0x820, sizeof(*out_a));
    __builtin_memcpy(out_b, (const char*)record + 0x824, sizeof(*out_b));
}

/* --- the scene-visibility wrapper band (see record.hpp) ---------------------
 * FUN_00473b40 (the corpus surface lock/unlock leaf) is the port's hook
 * boundary: the wrappers reproduce their own gates and stores and route the
 * leaf through the recorded no-op below. */
static int s_scene_vis_calls;
static void* s_scene_vis_last_obj;
static int s_scene_vis_last_flag;

static void scene_visibility_hook_call(void* object, int flag)
{
    ++s_scene_vis_calls;
    s_scene_vis_last_obj = object;
    s_scene_vis_last_flag = flag;
}

void scene_visibility_hook_reset(void)
{
    s_scene_vis_calls = 0;
    s_scene_vis_last_obj = NULL;
    s_scene_vis_last_flag = 0;
}

int scene_visibility_calls(void) { return s_scene_vis_calls; }

void scene_visibility_last_call(void** out_object, int* out_flag)
{
    if (out_object != NULL) {
        *out_object = s_scene_vis_last_obj;
    }
    if (out_flag != NULL) {
        *out_flag = s_scene_vis_last_flag;
    }
}

/* GraphicsOverlayVisibilitySet (FUN_004634b0) — the overlay words
 * DAT_00552180/DAT_0055218c (handle, cache) have no ported owner; they land
 * here as zero-init statics (deviation: corpus strides them blindly). */
static uint32_t s_overlay_word;  /* DAT_00552180 role */
static uint32_t s_overlay_cache; /* DAT_0055218c role */

void graphics_overlay_visibility_set(uint32_t flag)
{
    if (s_overlay_cache != flag) {
        s_overlay_cache = flag;
        /* corpus: the handle rides the call, flag folded to 1/0. */
        scene_visibility_hook_call((void*)(uintptr_t)s_overlay_word,
                                   flag != 0u ? 1 : 0);
    }
}

void graphics_overlay_word_set(uint32_t handle_word)
{
    s_overlay_word = handle_word;
}

uint32_t graphics_overlay_word_peek(void) { return s_overlay_word; }

/* SurfaceVisibilitySet (FUN_00464720) — record +0 handle through the leaf. */
void surface_visibility_set(const void* record, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(record, 0x00), (int)flag);
}

/* AnimationObjectVisibilitySet (FUN_00406c90) — animation record +4. */
void animation_object_visibility_set(const void* anim, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(anim, 0x04), (int)flag);
}

/* SceneObjectPairApplyMode (FUN_00408670) — the +4 object, then the +0x7c
 * partner when nonzero. Deviation: the partner test strides a dword in the
 * corpus; the port's widened slot answers through the pointer. */
void scene_object_pair_apply_mode(const void* record, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(record, 0x04), (int)flag);
    void* partner = rec_ptr(record, 0x7c);
    if (partner != NULL) {
        scene_visibility_hook_call(partner, (int)flag);
    }
}

/* AnimationObjectVisibilityGet (FUN_004086a0) — the +4 object's flag bit 3
 * through the ported getter. */
uint32_t animation_object_visibility_get(const void* anim)
{
    return scene_object_flag_bit3_get(rec_ptr(anim, 0x04));
}

/* SceneAssignEntityOwner (FUN_00408380) — owner store on the +4 object
 * through the ported owner set. */
void scene_assign_entity_owner(const void* scene, uint32_t owner)
{
    scene_object_owner_set(rec_ptr(scene, 0x04), owner);
}

/* SixResourceSlotsSetState (FUN_0046cfc0) — see record.hpp. */
void six_resource_slots_set_state(const void* record, uint32_t flag)
{
    for (int i = 0; i < 6; ++i) {
        void* handle = rec_ptr(record, 0x10 + (size_t)i * sizeof(void*));
        if (handle != NULL) {
            scene_visibility_hook_call(handle, (int)flag);
        }
    }
}

/* UIRecordCallbackForward (FUN_0045a630) — see record.hpp. */
void ui_record_callback_forward(const void* const* ui_record,
                                uint32_t* out_a, uint32_t* out_b)
{
    if (ui_record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    void* handle = rec_ptr(ui_record, 0x00);
    object_state_pair_get(handle, out_a, out_b);
}
