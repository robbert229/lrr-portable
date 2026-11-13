// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Flic.c's Flic_Close (flic.hpp). gods98/flic.h holds Panel.c's adapters.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declaration it pairs with has C linkage in place.
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) the C units' flics are
 * Flic.c's own (flic_unit.c, finding 632; flic_link_names.h gives it
 * flic_setup), so the macro below points their closes at the unit's
 * Flic_Close. The port's flic_close keeps its C++ callers. A macro renames
 * the declaration one for one, so MSVC's symbol numbering (finding 538)
 * does not change. */
#ifndef LEGORRR_GODS98_FLIC_API_H
#define LEGORRR_GODS98_FLIC_API_H

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
#define flic_close Flic_Close /* 00483f00 */
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Flic;
int flic_close(struct Flic* f);                             /* 00483f00 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FLIC_API_H */
