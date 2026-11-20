// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes Building.c's object (finding 693).
 *
 * The original's "LWO" literal (004a1d5c) follows Building.c's own strings
 * and precedes the screen-effect unit's, but no live code before
 * RockFallStyles.c reads it. String literals are pick-any COMDATs, so LINK
 * keeps the first object's copy: an unused `extern __inline` function here
 * used it, and /OPT:REF removed its code but not the literal that later
 * units still reference. Its original name is not known; nothing calls it. */
#ifndef LEGORRR_GAME_BUILDING_MODEL_TAIL_H
#define LEGORRR_GAME_BUILDING_MODEL_TAIL_H
extern __inline const char* building_model_tail_lwo(void) { return "LWO"; }
#endif
