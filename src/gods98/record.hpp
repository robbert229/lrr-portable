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

#endif /* LEGORRR_GODS98_RECORD_HPP */
