// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 sound layer: token split and sound-alias lookup.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * sound functions that units rebuilt as C call. The C++ code keeps
 * using gods98/sound.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GODS98_SOUND_H
#define LEGORRR_GODS98_SOUND_H

#ifdef __cplusplus
extern "C" {
#endif

int sound_variant_split(char* text, char** out, const char* separators);
                                                       /* FUN_00477440 */
int sound_alias_lookup(const char* name, int* out_index); /* FUN_00464c70 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_SOUND_H */
