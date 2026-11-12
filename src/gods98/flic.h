// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 Flic.c (original unit G98-016) entry points that units rebuilt as
 * C call, in the original ABI.
 *
 * The port's flic player (gods98/flic.hpp) has other contracts, so these are
 * thin unannotated adapters over it (ADR 0005 step 6), in gods98/flic.cpp.
 *
 * C boundary header (ADR 0005). Shared C++ headers do not include this file
 * (finding 506). */
#ifndef LEGORRR_GODS98_FLIC_H
#define LEGORRR_GODS98_FLIC_H

#ifdef __cplusplus
extern "C" {
#endif

struct Area2F;

/* FUN_00483c80 Flic_Setup: open `name` into *out. Nonzero on success. */
int flic_setup(const char* name, void** out, int flags);
struct Flic;

/* FUN_004897d0 / FUN_004850e0: the flic's frame width and height, with the
 * port's entries (gods98/flic.hpp, C linkage). */
int flic_width(const struct Flic* f);
int flic_height(const struct Flic* f);
/* FUN_00484070 Flic_Animate: advance and draw into `area`. */
int flic_animate(void* flic, const struct Area2F* area, int advance,
                 int trans);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FLIC_H */
