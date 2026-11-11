// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — callback record allocation (FUN_004628e0; findings
 * 366/393). The steered-group stack that this header used to declare
 * (FUN_0040f6c0-FUN_00410280) is now the original-shape unit GAME-016,
 * src/game/steering.c (finding 544).
 */
#ifndef LEGORRR_GODS98_PTRSTACK_HPP
#define LEGORRR_GODS98_PTRSTACK_HPP

#include <stdint.h>
#include <stddef.h>

/* CallbackRecordAllocateInitialize (FUN_004628e0): allocates the 0x28-byte
 * record, dwords zeroed, with dwords 0..5 = param_3/param_2/param_3/param_4/
 * param_5/param_6 (param_3 rides twice — pinned) and dword 8 (LP64: native)
 * holding a link slot whose first word is a zeroed 0x20-byte buffer. The
 * caller's out slot is written even when the record allocation fails
 * (pinned quirk), guarded for NULL (deviation). */
void* callback_record_allocate_initialize(void** out_record, uint32_t param_2,
                                          uint32_t param_3, uint32_t param_4,
                                          uint32_t param_5, uint32_t param_6);
                                                /* FUN_004628e0 */

#endif /* LEGORRR_GODS98_PTRSTACK_HPP */
