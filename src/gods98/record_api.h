// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * LegoObject.c's GetTypeAndIndex (record.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_RECORD_API_H
#define LEGORRR_GODS98_RECORD_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void copy_two_dword_pair_to_outputs(const void* record, uint32_t* out_a,
                                    uint32_t* out_b);       /* 00439930 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_RECORD_API_H */
