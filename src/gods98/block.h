// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 block-text config layer: the config boolean parser.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * block.cpp functions that units rebuilt as C call. The C++ code keeps
 * using gods98/block.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). The config tree
 * itself is gods98/config.h. */
#ifndef LEGORRR_GODS98_BLOCK_H
#define LEGORRR_GODS98_BLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

int block_get_bool(const char* value);                       /* FUN_00477710 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_BLOCK_H */
