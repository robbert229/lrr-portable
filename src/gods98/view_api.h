// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Images.c's Image_Remove (view.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_VIEW_API_H
#define LEGORRR_GODS98_VIEW_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_surface_release Image_Remove /* 0047d470 */
#endif
void image_surface_release(void* record);                  /* 0047d470 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_VIEW_API_H */
