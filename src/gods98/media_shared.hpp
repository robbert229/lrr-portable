// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/media.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_MEDIA_SHARED_HPP
#define LRR_GODS98_MEDIA_SHARED_HPP

extern int s_cursor_slot;
extern float s_cursor_override;

#endif /* LRR_GODS98_MEDIA_SHARED_HPP */
