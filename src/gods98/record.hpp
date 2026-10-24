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

#endif /* LEGORRR_GODS98_RECORD_HPP */
