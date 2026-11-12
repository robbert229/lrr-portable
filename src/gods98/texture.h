// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the texture.hpp functions that
 * original-shape units call. Shared C++ headers do not include it;
 * src/game/test_c_boundaries.cpp checks it against texture.hpp. */
#ifndef LEGORRR_GODS98_TEXTURE_H
#define LEGORRR_GODS98_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00473c60 (G98-002): the registered name of `name_base`'s part
 * `*frame_index` under the container. */
const char* xf_texture_name_format(void* texture_record,
                                   const char* name_base,
                                   const int* frame_index);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_TEXTURE_H */
