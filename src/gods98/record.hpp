// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — corpus record access primitives (findings 305/332/354/
 * 356/367/378/460). Small pure readers over caller-supplied records:
 * exact-offset field reads and the indexed slot tables. Like the entity
 * stats accessors, the field *meanings* stay with their callers (some
 * are unresolved per their findings); what is ported here is the
 * faithful read. All reads go through byte-safe accessors (the corpus's
 * dword/float reads can sit at offsets that are not pointer-aligned on
 * 64-bit hosts); where the corpus indexes, the index arithmetic is
 * reproduced verbatim. Where the corpus dereferences blindly the port
 * guards and answers 0 (documented deviation).
 */
#ifndef LEGORRR_GODS98_RECORD_HPP
#define LEGORRR_GODS98_RECORD_HPP

#include <stdint.h>
#include <stddef.h>

/* RecordField14Get (FUN_00435f0b, findings 03/354): the dword at +0x14.
 * The corpus's editor-camera record uses it through DAT_00556160. */
uint32_t record_field14_get(const void* record);

/* ObjectField10Get (FUN_004850e0, findings 04/378): the dword at +0x10. */
uint32_t object_field10_get(const void* object);

/* RecordFieldD8EqualsOne (FUN_0046c3d0, findings 367/432): true when the
 * int at +0xd8 is exactly 1. */
int record_field_d8_equals_one(const void* record);

/* ReadIndexedPairSlotDword (FUN_00471a20, findings 332/256): reads
 * base[i * 8 + j * 4] — the indexed pair-slot table. */
uint32_t read_indexed_pair_slot_dword(const void* base, uint32_t i,
                                      uint32_t j);

/* VelocitySlotPrimaryObjectGet (FUN_00471a40) /
 * VelocitySlotSecondaryObjectGet (FUN_00471a50) (finding 305): the
 * indexed dword slots at +0x18 / +0x24 (stride 4). The corpus reads
 * object references stored as 32-bit values; the port returns the raw
 * stored dword so no pointer is ever truncated. */
uint32_t velocity_slot_primary_object_get(const void* record,
                                          uint32_t index);
uint32_t velocity_slot_secondary_object_get(const void* record,
                                            uint32_t index);

/* CategoryValueIsNeitherZeroNorFifteen (FUN_0041b470, findings 252/356):
 * the value filter 0 / 0xf both fail. */
int category_value_is_neither_zero_nor_fifteen(int value);

/* InterpolatedZWithinBounds (FUN_00471330, findings 332/460): the
 * interpolated value b_plus8 * scale + a_plus8 answers in when
 * v <= target and target - t <= v. */
int interpolated_z_within_bounds(const void* record_a, const void* record_b,
                                 float t, float target, float scale);

/* CameraRotationGet (FUN_00436b79, findings 339/426): the float at
 * camera +0x84. The axis/units of the field stay with the callers
 * (finding 426: not established from the corpus); this is the faithful
 * read, as in the stats accessors. NULL answers 0. */
float camera_rotation_get(const void* camera);

/* MapCellToWorldCenter (FUN_0044f640, finding 225): bounds-check a cell
 * against the map descriptor and answer the cell's world X/Z center.
 * The descriptor's five fields are int dwords the corpus converts to
 * float on the fly: width +0, height +4, scale +16, origin X +24,
 * origin Y +28. The bounds test is the corpus's UNSIGNED compare
 * (negative indices fail as huge ones); NULL answers 0. */
int map_cell_to_world_center(const void* map, uint32_t cell_x,
                             uint32_t cell_y, float* world_x, float* world_y);

/* TaskFlagBitQuery (FUN_00408970, findings 366/465): test bit `bit & 31`
 * of the task record's flag word at +0x98. A nonzero suppress answers
 * 0 when the bit is SET (the corpus's "skip if flagged and asked not
 * to" form), otherwise the mask value. */
uint32_t task_flag_bit_query(const void* task, uint32_t bit, int suppress);

/* TaskField84GetAndField88Out (FUN_00408360, finding 366): return the
 * dword at +0x84 and, when given an output pointer, copy the dword at
 * +0x88 there. Field meanings stay with the task layer. NULL task
 * answers 0 and writes nothing (documented deviation). */
uint32_t task_field84_get_and_field88_out(const void* task, uint32_t* out88);

/* PackedGridCellValueGet (FUN_0040b850, findings 373/451): decode a
 * packed cell byte — the low nibble steps by the descriptor's width
 * (word +0), the high nibble is the short step — and read the grid
 * dword at (descriptor +8) + index*4. NULL answers 0, matching the
 * corpus's own guard. Deviation: the corpus's 32-bit cell-array slot is
 * read as a native pointer (the port's model of a corpus pointer
 * field). */
uint32_t packed_grid_cell_value_get(const void* grid, uint32_t packed_cell);

/* ObjectPoolListHeadGet (FUN_00456c30, finding 374): the list-head slot
 * table — index -1..0x37 (exclusive) over +0x114, with the corpus's
 * "zero answers zero" fold. Out of range or NULL answers 0. */
int object_pool_list_head_get(const void* pools, int index);

/* ModelChildRenderEligibilityCheck (FUN_00483a70, finding 431): the
 * render predicate — eligible when NEITHER bit 0x1 nor bit 0x10000000
 * of the +0x6c flag word is set. NULL answers 0 (documented
 * deviation). */
int model_child_render_eligibility_check(const void* object);

/* EntityTypeFlagBit1Get (FUN_0046d330, findings 357/432): bit 1 of the
 * flag dword at +0x1e8; the flag's meaning stays with the callers per
 * finding 357. NULL answers 0. */
uint32_t entity_type_flag_bit1_get(const void* record);

/* ------------------------------------------------------------------ *
 *  Field-reader sweep (2026-10-24; findings 305/337/356/361/372/373/
 *  376/381/393/411/430/432/435). Opaque-record faithful reads — the
 *  field MEANINGS stay with the callers per each function's finding.
 *  NULL records answer 0 (documented deviation); indexed reads follow
 *  the corpus's pointer slot and stride arithmetic, reading corpus
 *  pointer fields native-size as elsewhere in this module.
 * ------------------------------------------------------------------ */

/* ObjectRecordField11CGet (FUN_0046db60, findings 372/432): +0x11c. */
uint32_t object_record_field11c_get(const void* record);

/* ObjectFieldBE4DwordGet (FUN_0047a540, findings 361/411): +0xbe4. */
uint32_t object_field_be4_dword_get(const void* record);

/* EntityTypeMaskGet (FUN_0046cf80, findings 357/432): dword at +300. */
uint32_t entity_type_mask_get(const void* record);

/* PointerValueGet (FUN_00482210, findings 381/411): the dword it points
 * at (offset +0 of the passed pointer). */
uint32_t pointer_value_get(const void* pointed);

/* SceneObjectFlagsGet (FUN_00408850, findings 373/435): +0x70. */
uint32_t scene_object_flags_get(const void* scene_object);

/* SceneObjectOwnerGet (FUN_00473db0, finding 430): +0x20. */
uint32_t scene_object_owner_get(const void* scene_object);

/* SceneObjectResourceGet (FUN_0046d9e0, findings 357/432): +0x08. */
uint32_t scene_object_resource_get(const void* scene_object);

/* SceneObjectWrappedPointerGet (FUN_00473680, findings 337/430): the
 * wrapped pointer's contents (the target's dword at +0). */
uint32_t scene_object_wrapped_pointer_get(const void* wrapped);

/* SceneObjectFlagBit3Get (FUN_00473ba0, findings 305/430): bit 3 of the
 * flag byte at +0x14. */
uint32_t scene_object_flag_bit3_get(const void* scene_object);

/* SceneObjectModeIsOne (FUN_00406980, findings 373/435): the +0x28 mode
 * word equals 1. */
int scene_object_mode_is_one(const void* scene_object);

/* AnimationObjectHandleGet (FUN_00406e30, findings 356/435): +0x04. */
uint32_t animation_object_handle_get(const void* animation_object);

/* ResourceEntryIdGet (FUN_004877d0, findings 381/416): the 16-bit entry
 * id word at +2 (zero-extended, as the corpus returns it). */
uint32_t resource_entry_id_get(const void* entry);

/* IndexedWordTableLookup (FUN_004850c0, findings 373/431): word table
 * CORPUS ORDER — (index, table): (index & 0xff) steps 2 bytes from the
 * table's +0x41c start. */
uint32_t indexed_word_table_lookup(uint32_t index, const void* table);

/* IndexedRecordFlag6CBit0Get (FUN_00482780, findings 373/431): bit 0 of
 * the indexed record's +0x6c word — base table from the record's +0x8
 * pointer slot, stride 0x70. */
uint32_t indexed_record_flag6c_bit0_get(const void* record, int index);

/* IndexedRecordDataAddressGet (FUN_00483400, findings 372/431): the
 * INDEXED RECORD's data address, base + 0x10 + index*0x70 — the address
 * itself, not its contents (as the corpus answers). NULL table answers
 * NULL (documented deviation). */
void* indexed_record_data_address_get(const void* record, int index);

/* CountMatchingCoordinatesInList (FUN_00408e10, findings 376/393): the
 * coordinate-pair count — `count` (word +0x20) entry pointers stored
 * inline from +0x2c, each entry's first two dwords tested. Deviation:
 * the inline entry slots hold native pointers. */
int count_matching_coordinates_in_list(const void* manager,
                                       int32_t coord_x, int32_t coord_y);

/* AviPlayerReadyGet (FUN_0047ed80, findings 185/414): the movie
 * object's ready BYTE at +0x99 — one when the AVI open chain succeeded,
 * zero otherwise. Byte-safe read (the offset straddles dwords); NULL
 * answers 0. */
uint32_t avi_player_ready_get(const void* movie);

/* GameFileSystemTypeGet (FUN_0047fe20, findings 234/404): the system-kind
 * dword at the very start of the game-file handle — callers choose WAD
 * versus standard operations with it. Faithful read; NULL answers 0. */
uint32_t game_file_system_type_get(const void* handle);

/* IndexedRecordTransformCopyIn (FUN_00483240, findings 372/431): copy
 * 0x50 bytes (20 dwords) from `source` into the INDEXED RECORD's inline
 * block — the same +0x8 table slot and base+0x10+index*0x70 address the
 * data-address reader above hands back — and return 1. The corpus's
 * dword-at-a-time loop is the contiguous copy below (byte-equivalent,
 * the crt memmove precedent); NULL record or source stores nothing and
 * still answers success (documented deviation). */
uint32_t indexed_record_transform_copy_in(void* record, int index,
                                          const void* source);

/* --- grid-descriptor coordinate leaves (2026-10-24) ---------------------
 * The map descriptor map_cell_to_world_center reads — width +0x00, height
 * +0x04, scale +0x10, origins +0x18 / +0x1C (int fields converted at the
 * use site) — gains its inverse pair. Truncations are toward zero (the
 * corpus's __ftol under the ported control word, the crt precedent);
 * NULL descriptors skip the answer (documented deviation; corpus callers
 * pass live pointers). */

/* WorldToTerrainCellBoundsCheck (FUN_0044f700): truncate the world point
 * to cell coordinates — ALWAYS stored, NULL-safe — then check -1 < c < d.
 * In range, answers 1 and (when given) writes the subcell coordinate: the
 * center-relative position in half-cell units, folded past 1 by
 * 2-x-with-lowest. Out of range answers 0. */
int world_to_terrain_cell_bounds_check(const void* map, float world_x,
                                       float world_y, int* cell_x,
                                       int* cell_y, float* subcell);

/* GridCoordinateClampAndStore (FUN_0044f810): truncate a world point to
 * cells and clamp each into 0..dim-1 before storing (NULL-safe stores). */
void grid_coordinate_clamp_and_store(const void* map, float world_x,
                                     float world_y, int* cell_x, int* cell_y);

/* TerrainWorldPointToCell (FUN_0044f870): the grid-pick forwarder's twin
 * call shape: check the point through the bounds check (the forwarder
 * drops the scale answer), and while in range write the cell coordinates
 * as floats, and — when an offset is asked for — the point's fractional
 * place in the cell (point minus the center, halved-cell-scaled plus a
 * half-step, the corpus's - -0.5). The scale divisor reads +0x10 as a
 * FLOAT here (that leaf's own pseudocode), while the center formula
 * converts the same field as an int — both reproduced faithfully; a
 * zero scale divides like the corpus does (infinity answers). Always
 * answers 0. */
uint32_t terrain_world_point_to_cell(const void* map, const float* point,
                                     float* cell_out, float* offset_out);

/* --- indexed flag slot and resource-handle writers (2026-10-24) ---------
 * Pure offset writers on the opaque records, modeled on the module's
 * reader twins (rec_dword reads, memcpy stores). NULL records skip the
 * store (documented deviation; corpus callers dereference blindly). */

/* IndexedFlagValueSetOne (FUN_0044e6b0, findings 354/450): write dword 1
 * at record + 0x40b4 + (index & 0xff)*4 — the low byte selects the flag
 * slot in the record's indexed flag array. */
void indexed_flag_value_set_one(void* record, uint32_t index);

/* SceneObjectResourceHandleSet (FUN_0044f220, finding 373): store the
 * resource handle at object + 0x2c. */
void scene_object_resource_handle_set(void* object, uint32_t handle);

/* --- entity record-field leaves (2026-10-24) -------------------------------
 * RecordFields4And14Set (FUN_00435ef4, findings 354/476): store the two
 * supplied dwords at record +4 and +0x14 (the latter pairs with the
 * ported RecordField14Get read leaf).
 * EntityFieldsA0A4StoreA8Clear (FUN_00435f32, finding 476): store the two
 * supplied dwords at +0xa0/+0xa4 and clear +0xa8.
 * EntityB8Bit3SetOrClear (FUN_00435d91, finding 476): raise or clear bit
 * 3 of the +0xb8 byte. Field meanings stay unresolved (findings 354);
 * NULL records skip the stores (documented deviation; corpus callers
 * dereference blindly). The corpus writes blindly into whatever record
 * its callers hand in; the port keeps the write shape. */
void record_fields_4_and_14_set(void* record, uint32_t value_a,
                                uint32_t value_b);
void entity_fields_a0_a4_store_a8_clear(void* record, uint32_t value_a,
                                        uint32_t value_b);
void entity_b8_bit3_set_or_clear(void* record, int set);

#endif /* LEGORRR_GODS98_RECORD_HPP */
