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

/* VelocitySlotVectorRead (FUN_00471c70): copies the three float words at
 * +0x44 + index * 12 into the caller's vector. A NULL record reads three
 * zero words; the port guards the output slot (deviation: the corpus
 * strides the caller's buffer and the record blindly). */
void velocity_slot_vector_read(const void* record, float out[3],
                               uint32_t index);

/* EntityFlag80Set (FUN_00473b20): raises the record's +0x14 flag word's
 * bit 7 when set is nonzero, drops it otherwise. The port guards the
 * record (deviation: the corpus strides it blindly). */
void entity_flag_80_set(void* record, int set);

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
/* ObjectPoolListsFreeAll (FUN_00456c60, finding 374): frees every chain
 * hanging off the pool table's 0x37 list heads — the corpus's head slots
 * stride as dwords at +0x114 in the 0x284-byte level block; the port
 * models each head as a native-size link slot (the widened-link
 * convention), so the table passed here is a port-layout image and the
 * head stores survive real pointers. Each chain walks the node's +0x08
 * next link through the engine heap, then the head is cleared. The corpus
 * threads the level-block global (CurrentLevelGet); the port threads the
 * table as an argument.
 * Deviations: the corpus strides the head blindly (the port guards it);
 * object_pool_list_head_get keeps the corpus-layout dword view, a view
 * this port storage does not share. */
void object_pool_lists_free_all(void* pools); /* FUN_00456c60 */


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

/* --- the record-field write band (2026-10-24; findings 324/386) ------------
 * Pure field writers over record handles, in the port's fixed-offset write
 * style. NULL records skip the stores (documented deviations; corpus
 * callers dereference blindly).
 * RecordField18Set (FUN_00462ab0): dword store at +0x18.
 * RecordFlags24Or (FUN_00462ac0): OR the flag word at +0x24.
 * ObjectField14Set (FUN_0045d8a0): dword store at +0x14 (the corpus's
 *   generic object/record field write).
 * LinkedRecordField4Clear (FUN_00451c30): walk the record chain through
 *   each node's +0x14 next link, clearing every node's +4 word - faithful
 *   chain walk with an entry NULL guard (documented). */
void record_field18_set(void* record, uint32_t value);
void record_flags_24_or(void* record, uint32_t flags);
void object_field14_set(void* record, uint32_t value);
void linked_record_field4_clear(void* record);

/* CopyTwoDwordPairToOutputs (FUN_00439930): split a two-dword record
 * field across two output words — the corpus's blind dereferences get
 * entry NULL guards (documented). */
void copy_two_dword_pair_to_outputs(const void* record, uint32_t* out_a,
                                    uint32_t* out_b);

/* SceneObjectOwnerSet (FUN_00473da0): dword store at +0x20 — the port's
 * fixed-offset style, entry-guarded (documented deviation). */
void scene_object_owner_set(void* scene_object, uint32_t owner);

/* --- the record primitive band (2026-10-24; findings 393/386) ---------------
 * IndexedRecordField64Set (FUN_00482350): dword store through the
 *   record's +8 base pointer at base + 100 + index * 0x70.
 * ObjectVector3Set (FUN_0040ff70): three float stores at base + 0x3c…44
 *   where the object block's slot 0 holds the base pointer.
 * FloatPairAtOffset4SetFromInts (FUN_004851b0) / …OffsetCSetFromUInts
 *   (FUN_004851d0): integer-to-float pair stores at +4/+8 (signed) and
 *   +0xc/+0x10 (unsigned) — the corpus's casts faithful.
 * ObjectStatePairGet (FUN_00485a00): read the +0x820/+0x824 state pair.
 * Entry NULL guards stand in for the corpus's blind dereferences
 * (documented deviations). */
void indexed_record_field64_set(void* record, int index, uint32_t value);
void object_vector3_set(void* object_block, float x, float y, float z);
void float_pair_at_offset4_set_from_ints(void* record, int x, int y);
void float_pair_at_offsetc_set_from_uints(void* record, uint32_t x,
                                          uint32_t y);
void object_state_pair_get(const void* record, uint32_t* out_a,
                           uint32_t* out_b);

/* --- scene-visibility wrapper band (2026-10-24; findings 446/370/435/356/
 * 373/476/28) --------------------------------------------------------------
 * The corpus's visibility leaf (FUN_00473b40, the surface lock/unlock) is
 * the port's hook boundary — quoted in tickers.cpp, no body here — so the
 * wrappers below reproduce their own gates and field work and route the
 * leaf through the documented no-op hook (the simtick drop-leaf precedent).
 * Handle slots that the corpus fills with record addresses are read as
 * native pointer slots (the module's established widening).
 * GraphicsOverlayVisibilitySet (FUN_004634b0): the overlay word changes
 *   only when the flag differs from the cached value; the handle rides
 *   the call through unchanged. Both corpus words are port statics.
 * SurfaceVisibilitySet (FUN_00464720): record +0 handle, flag through.
 * AnimationObjectVisibilitySet (FUN_00406c90): animation record +4.
 * SceneObjectPairApplyMode (FUN_00408670): the +4 object, then the +0x7c
 *   partner when it is nonzero.
 * AnimationObjectVisibilityGet (FUN_004086a0): the +4 object's flag bit 3
 *   through the ported getter.
 * SceneAssignEntityOwner (FUN_00408380): owner store on the +4 object
 *   through the ported owner set. */
void scene_visibility_hook_reset(void);             /* port hook */
int  scene_visibility_calls(void);                  /* port hook */
void scene_visibility_last_call(void** out_object, int* out_flag); /* hook */
void graphics_overlay_visibility_set(uint32_t flag);
                                                /* FUN_004634b0 */
void graphics_overlay_word_set(uint32_t handle);    /* port hook */
uint32_t graphics_overlay_word_peek(void);          /* port hook */
void surface_visibility_set(const void* record, uint32_t flag);
                                                /* FUN_00464720 */
void animation_object_visibility_set(const void* anim, uint32_t flag);
                                                /* FUN_00406c90 */
void scene_object_pair_apply_mode(const void* record, uint32_t flag);
                                                /* FUN_00408670 */
/* SceneObjectPairSetRenderParameters (FUN_004086c0): the pair's pose
 * dispatch lands on the ported position + facing primitives — the corpus
 * hands the dispatch (x, y, 0) plus the constant up vector (0, 0, -1),
 * which the port reads as the position (x, y, 0) and facing (0, 0, -1).
 * SceneObjectPairSetCallbackScalar (FUN_00408710): run the scalar probe
 * (the sole call-site probe is the ported grid-operation height probe,
 * which answers void) and set each handle's position to (x, y, 0) — the
 * corpus consumes the probe's undefined float return as the z; the port
 * pins it to zero. Both stride the +4 handle, then the +0x7c partner
 * when non-NULL. */
void scene_object_pair_set_render_parameters(void* pair, float x, float y);
                                                /* FUN_004086c0 */
typedef void (*SceneScalarProbe)(float x, float y, void* arg);
void scene_object_pair_set_callback_scalar(void* pair, float x, float y,
                                           SceneScalarProbe probe,
                                           void* arg); /* FUN_00408710 */
uint32_t animation_object_visibility_get(const void* anim);
                                                /* FUN_004086a0 */
void scene_assign_entity_owner(const void* scene, uint32_t owner);
                                                /* FUN_00408380 */

/* SceneCurrentObjectSet (FUN_00477240): the interface record's current-
 * object slot at +0x14. Its consumer (the slot-dispatch clear pair) stays
 * on the interface seam, so the writer rides the widened pointer slot with
 * the corpus's dword stride documented as a deviation. */
void scene_current_object_set(void* record, void* object);
                                                /* FUN_00477240 */

/* SixResourceSlotsSetState (FUN_0046cfc0, findings 354/432): the record's
 * six resource slots from +0x10 — each nonzero handle rides the visibility
 * leaf with the flag. Deviation: the slots are handled through the module's
 * widened pointer model (stride native), matching the other handle slots. */
void six_resource_slots_set_state(const void* record, uint32_t flag);
                                                /* FUN_0046cfc0 */

/* UIRecordCallbackForward (FUN_0045a630, findings 386/439): dereferences the
 * UI record's +0 handle and forwards the two outputs through the ported
 * state-pair read. Widened handle slot like the other handle rows. */
void ui_record_callback_forward(const void* const* ui_record,
                                uint32_t* out_a, uint32_t* out_b);
                                                /* FUN_0045a630 */

/* RecordFlagBitTest (FUN_0046cf40, findings 367/432): the selected bit of
 * the record's +0x130 word, optionally rejecting bits already raised in
 * the associated type mask. The NULL guards are the documented deviations. */
uint32_t record_flag_bit_test(const void* record, uint32_t bit_index,
                              int reject_masked);   /* FUN_0046cf40 */

/* --- the UI record factories (2026-10-24; findings 305/397) -----------------
 * BoundedFloatRecordCreate (FUN_00410b90): a `kind` record bounded to the
 *   open interval (min, max) — rejected when min >= max — carrying six
 *   payload words through the engine heap.
 * MenuItemNumericRangeRecordAllocate (FUN_00410bf0): the slider variant's
 *   wider record: kind + int bounds + twelve payload words through the same
 *   gate. Both answer NULL on rejection or heap failure.
 * Deviation: the corpus strides its windows as dwords; both records land
 * in the port's record window (field n at record + n*sizeof(void*)). */
void* bounded_float_record_create(int kind, float min_v, float max_v,
                                  uint32_t f3, uint32_t f4, uint32_t f5,
                                  uint32_t f6);                      /* FUN_00410b90 */
void* menu_item_numeric_range_record_allocate(int kind, int min_v, int max_v,
                                              uint32_t f3, uint32_t f4,
                                              uint32_t f5, uint32_t f6,
                                              uint32_t f7, uint32_t f8,
                                              uint32_t f9, uint32_t f10,
                                              uint32_t f11, uint32_t f12,
                                              uint32_t f13, uint32_t f14);
                                                                    /* FUN_00410bf0 */

/* NestedRecordListRemoveByKeyRecursive (FUN_0047b2e0): the deep-first key
 * sweep of a nested list. The recursion descends through the node's next
 * link; a true child means the child's own successor gets promoted into
 * its slot and the child is freed through the engine heap; the node then
 * answers whether its key (+0x08) matches.
 * Deviations: the corpus strides its dword links blindly (the port keeps
 * the next link as a native pointer per the widened-link convention); the
 * corpus's first argument never reads (calling-convention residue). */
int nested_record_list_remove_by_key_recursive(int key, void* node);
                                                /* FUN_0047b2e0 */

/* --- indexed-record mesh window band (2026-10-25) ------------------------
 * The stride-0x70 blocks the indexed record's +0x8 table slot already
 * strides, read through the block window the corpus established: +0x4 is
 * the vertex count (re-read every round, as the corpus's aliasing reloads
 * do), +0xc is the vertex-base slot (widened native pointer), vertices
 * stride 0x20 as {position, normal, uv pair}. The +0x6c flag word and the
 * data window (base + 0x10 + index*0x70) keep the readers ported above.
 * Deviations: the corpus strides the table, vertex base, and every window
 * blindly — the port guards NULL table/base slots and stores nothing;
 * float math runs in single precision (documented x87 deviation). */

/* MeshVerticesScaleApply (FUN_00482370): scale every block's vertices —
 * x times the first component, y the second, z the third. Outer (block)
 * count at record +0x0 and inner vertex count re-read every round; a
 * zero-count block is skipped before its do-while ever runs. */
void mesh_vertices_scale_apply(void* record, float scale_x, float scale_y,
                               float scale_z);                     /* FUN_00482370 */

/* IndexedGeometryDataCopyToOutput (FUN_00482470): pack `count` rounds of
 * two whole vertices plus the first two dwords of the next from the
 * block's vertex base (starting at first_vertex) into 36-byte output
 * slots — the corpus's stride, with the fourth dword of each slot left
 * untouched. */
void indexed_geometry_data_copy_to_output(void* record, int block,
                                          int first_vertex, int count,
                                          void* output);            /* FUN_00482470 */

/* InstanceQuadVertexDataFill (FUN_00482500): fill the block's vertex
 * window — positions copied from the source window, the normal window
 * recomputed as (triple - position) and normalized in place (a zero
 * normal answers inf/NaN exactly as the corpus's division does), the uv
 * slot taken from the advancing pair stream. The triple is read every
 * round without advancing: the same triple shapes every vertex (pinned
 * corpus quirk). */
void instance_quad_vertex_data_fill(void* record, int block,
                                    int first_vertex, int count,
                                    const float* positions,
                                    const float triple[3],
                                    const uint32_t* pairs);         /* FUN_00482500 */

/* MeshSegmentVertexFill (FUN_00482620): the segment twin — positions and
 * pairs advance round by round while the triple stays exactly where it
 * was, so every vertex receives the SAME triple (the pinned quirk this
 * family is known for). */
void mesh_segment_vertex_fill(void* record, int block, int first_vertex,
                              int count, const float* positions,
                              const uint32_t triple[3],
                              const uint32_t* pairs);               /* FUN_00482620 */

/* MeshVertexRecordFill (FUN_004826c0): the filling twin — the triple
 * stream is an array of pointers into three-dword records, advanced one
 * entry per round, so each vertex gets its own triple (the counterpart of
 * the segment quirk). */
void mesh_vertex_record_fill(void* record, int block, int first_vertex,
                             int count, const float* positions,
                             const void* const* triples,
                             const uint32_t* pairs);                /* FUN_004826c0 */

/* IndexedRecordTransformInitialize (FUN_00483170): lay the identity
 * transform — {size 0x50, 1.0f x 4, sixteen zero dwords} — over the
 * data-address window and answer 1 (the corpus's dword loop is the
 * contiguous copy the transform-copy precedent established). */
uint32_t indexed_record_transform_initialize(void* record, int index);
                                                /* FUN_00483170 */

/* KeyedResourceEntryRemove (FUN_0047b2a0): the keyed sweep of the
 * association list. The deep-first key cleanup runs from the list head
 * (the port hook, the corpus's unowned global); a matched head promotes
 * its successor and frees through the engine heap.
 * Deviations: the corpus strides the head and its dword links blindly
 * (the port guards and widens); the sweep keeps the port's sound-buffer
 * association node layout — field 2 carries the key word, field 3 the
 * link — since that is where the corpus's global lives. */
void keyed_resource_entry_remove(int key);      /* FUN_0047b2a0 */

/* ScreenEffectItemOpacitySet (FUN_00483420): writes a clamped opacity
 * into the indexed record's effect window — mode 4 composes the +0x6c
 * flag word from the item's +0x64 word and the record's +0x18 feature
 * word (bits 0x8000/0x10000000, the corpus's own precedence pinned),
 * mode 5 lands the plain +0x44 dword, and every other mode answers 1
 * without a trace. Deviations: NULL windows store nothing yet still
 * answer 1; the corpus reads its two flag words blindly. */
uint32_t screen_effect_item_opacity_set(void* record, int block,
                                        float opacity, int mode);
                                                /* FUN_00483420 */

/* RenderObjectColorVectorSet (FUN_00483270): writes a clamped RGB triple
 * into one of the indexed record's four color windows (slot 0 doubles as
 * the white-opacity flag: bit 0x100000 raises when every channel reads
 * exactly 1.0). Deviations: NULL table windows store nothing yet still
 * answer 1; the corpus's clamps run before every branch, NaN included. */
uint32_t render_object_color_vector_set(void* record, int block, float red,
                                        float green, float blue, int slot);
                                                /* FUN_00483270 */

#endif /* LEGORRR_GODS98_RECORD_HPP */
