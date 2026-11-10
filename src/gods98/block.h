// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 block-text config layer: record lookup and accessors.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * config functions that units rebuilt as C call. The C++ code keeps
 * using gods98/block.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GODS98_BLOCK_H
#define LEGORRR_GODS98_BLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BlockFile BlockFile;
typedef struct BlockRec BlockRec;

BlockRec* block_find(BlockFile* bf, const char* path);       /* FUN_004792e0 */
BlockRec* block_subgroup(BlockFile* bf, const char* path);   /* FUN_00478ff0 */
const char* block_name(const BlockRec* r);
const char* block_value(const BlockRec* r);
BlockRec* block_first_child(const BlockRec* r);
BlockRec* block_next_equal_key(const BlockRec* r);           /* FUN_00479020 */
const char* block_get_strptr(BlockFile* bf, const char* path); /* FUN_004790b0 */
int block_get_bool(const char* value);                       /* FUN_00477710 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_BLOCK_H */
